// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtWayland.Compositor

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

AppletDockItem {
    id: root

    readonly property bool fashionMode: Panel.fashionMode
    readonly property int dockSize: Panel.rootObject.dockSize
    readonly property int hoverInset: 3
    readonly property real taskbarRadius: Panel.rootObject.fashionDock.backgroundRadius
    readonly property real hoverBackgroundRadius: taskbarRadius - hoverInset
    readonly property int adaptiveCardLeftWidth: 150 + Math.max(0, dockSize / 4)
    readonly property int rightContentPadding: Math.max(10, Math.round(adaptiveCardLeftWidth * 0.07))
    readonly property int verticalInset: Math.max(5, Math.round(dockSize * 0.16))
    readonly property int pageContentHeight: Math.max(24, dockSize - verticalInset * 2)

    dockOrder: 5
    shouldVisible: fashionMode && pageCount > 0
    readonly property int pageCount: sortedCards.count
    property bool contentHovered: false
    readonly property bool effectiveHovered: rootHoverHandler.hovered || contentHovered

    // The card the user was looking at is persisted so it can be restored after
    // the dock restarts.  The surface id ("pluginId::itemKey") is stored instead
    // of the page index, because every card comes from its own plugin process
    // and the surfaces may be created in a different order on each startup.
    property bool currentRestored: false
    // Reordering the pages moves the current card around, and removing a card
    // shifts the pages after it.  Such index changes must not be persisted,
    // only the ones caused by the user switching cards.
    property bool modelChanging: false

    // Card surfaces show up in whatever order their plugin processes manage to
    // connect to the compositor, so they are sorted by the order each plugin
    // reports before being displayed.
    readonly property int sourceCount: DockCompositor.cardPluginSurfaces.count

    visible: shouldVisible
    implicitWidth: adaptiveCardLeftWidth
    implicitHeight: dockSize
    clip: true

    function updateContentHovered() {
        const item = swipeView.currentItem
        contentHovered = !!item && item.surfaceHovered
    }

    function surfaceIdOf(surface) {
        return surface ? `${surface.pluginId}::${surface.itemKey}` : ""
    }

    function cardSurfaceId(index) {
        if (index < 0 || index >= sortedCards.count) {
            return ""
        }

        return surfaceIdOf(sortedCards.get(index).shellSurface)
    }

    function cardIndexOf(surfaceId) {
        if (!surfaceId) {
            return -1
        }

        for (let i = 0; i < sortedCards.count; ++i) {
            if (cardSurfaceId(i) === surfaceId) {
                return i
            }
        }
        return -1
    }

    // Cards are sorted by the order their plugin reports, ascending, so the
    // smallest one is shown first.  A plugin that does not care reports a very
    // large value and lands at the back, and so does a card whose order has not
    // arrived yet, since it is reported right after the surface is created.
    // The sort is stable, cards sharing an order stay in creation order.
    function desiredSurfaces() {
        const source = DockCompositor.cardPluginSurfaces
        const cards = []

        for (let i = 0; i < source.count; ++i) {
            const surface = source.get(i).shellSurface
            if (!surface) {
                continue
            }

            cards.push({ surface: surface, order: surface.cardOrder, created: i })
        }

        cards.sort((a, b) => a.order - b.order || a.created - b.created)
        return cards.map(entry => entry.surface)
    }

    // Brings sortedCards in line with the compositor model.  The rows are
    // inserted, removed and moved one by one instead of being rebuilt, so the
    // delegates - and with them the wayland surface items - are kept alive.
    function syncSortedCards() {
        const desired = desiredSurfaces()

        for (let i = sortedCards.count - 1; i >= 0; --i) {
            if (desired.indexOf(sortedCards.get(i).shellSurface) < 0) {
                sortedCards.remove(i)
            }
        }

        for (let target = 0; target < desired.length; ++target) {
            const surface = desired[target]
            let current = -1
            for (let i = target; i < sortedCards.count; ++i) {
                if (sortedCards.get(i).shellSurface === surface) {
                    current = i
                    break
                }
            }

            if (current < 0) {
                sortedCards.insert(target, { shellSurface: surface })
            } else if (current !== target) {
                sortedCards.move(current, target, 1)
            }
        }

        syncCurrentCard()
    }

    // Restores the card persisted by the previous session, and keeps that card
    // visible while cards are added to, removed from or reordered in the model.
    function syncCurrentCard() {
        modelChanging = false

        const savedIndex = cardIndexOf(Panel.cardCurrent)
        if (savedIndex >= 0) {
            currentRestored = true
            if (swipeView.currentIndex !== savedIndex) {
                swipeView.setCurrentIndex(savedIndex)
            }
            return
        }

        // Either nothing has been persisted yet, or the persisted card is not
        // available.  During startup its plugin process may still be coming up,
        // so only fall back to the current card once the deadline has passed.
        if (!Panel.cardCurrent || currentRestored) {
            currentRestored = true
            saveCurrentCard()
        }
    }

    function saveCurrentCard() {
        // Saving before the restore finished would overwrite the persisted card
        // with whichever card surface happens to be created first.
        if (!currentRestored || modelChanging) {
            return
        }

        const id = cardSurfaceId(swipeView.currentIndex)
        if (id) {
            Panel.cardCurrent = id
        }
    }

    onSourceCountChanged: {
        if (sourceCount === 0) {
            // All card plugins are gone (e.g. their processes were restarted),
            // restore again instead of persisting the first card that returns.
            currentRestored = false
        }

        modelChanging = true
        syncTimer.restart()
    }
    Component.onCompleted: syncTimer.restart()

    // Sorted view of DockCompositor.cardPluginSurfaces, see syncSortedCards().
    ListModel {
        id: sortedCards
    }

    // The card surfaces are created one by one by independent plugin processes.
    // Coalesce the updates, and let the SwipeView pick up the model change
    // before touching currentIndex, otherwise setCurrentIndex() would be
    // clamped to the old page count.
    Timer {
        id: syncTimer

        interval: 50
        repeat: false
        onTriggered: root.syncSortedCards()
    }

    // Stop waiting for a card that never appears (e.g. its plugin was removed),
    // otherwise the current card would never be persisted again.
    Timer {
        id: restoreTimeout

        interval: 10000
        running: !root.currentRestored
        onTriggered: {
            root.currentRestored = true
            root.saveCurrentCard()
        }
    }

    HoverHandler {
        id: rootHoverHandler
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
    }

    AppletItemBackground {
        x: root.hoverInset
        y: root.hoverInset
        width: parent.width - root.hoverInset
        height: parent.height - root.hoverInset * 2
        radius: root.hoverBackgroundRadius
        enabled: false
        opacity: root.effectiveHovered ? 1 : 0
        D.ColorSelector.hovered: root.effectiveHovered

        Behavior on opacity {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutCubic
            }
        }
    }

    Item {
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        visible: root.effectiveHovered && root.pageCount > 1
        width: pageIndicator.implicitHeight
        height: pageIndicator.implicitWidth

        PageIndicator {
            id: pageIndicator

            anchors.centerIn: parent
            rotation: 90
            count: swipeView.count
            currentIndex: swipeView.currentIndex
            padding: 0
            spacing: 4

            delegate: Rectangle {
                required property int index

                implicitWidth: 2
                implicitHeight: 2
                radius: width / 2
                color: Panel.colorTheme === Dock.Dark
                    ? Qt.rgba(0, 0, 0, index === pageIndicator.currentIndex ? 0.6 : 0.2)
                    : Qt.rgba(1, 1, 1, index === pageIndicator.currentIndex ? 0.6 : 0.2)
            }
        }
    }

    SwipeView {
        id: swipeView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 8
        anchors.rightMargin: root.rightContentPadding
        height: root.pageContentHeight
        orientation: Qt.Vertical
        interactive: count > 1
        clip: true

        onCurrentItemChanged: {
            root.updateContentHovered()
            surfaceGeometryUpdateTimer.restart()
        }

        onCurrentIndexChanged: {
            // The popups belong to the card that was on screen, drop them when
            // the user swipes to another one.
            cardToolTip.close()
            cardMenu.close()
            root.saveCurrentCard()
        }

        Repeater {
            model: sortedCards

            delegate: Item {
                id: surfaceHost

                property var plugin: model.shellSurface
                readonly property bool surfaceHovered: SwipeView.isCurrentItem && surfaceItem.hovered

                ShellSurfaceItemProxy {
                    id: surfaceItem
                    width: parent.width
                    height: parent.height
                    shellSurface: surfaceHost.plugin
                }

                function updateSurfaceGeometry() {
                    if (!plugin || !SwipeView.isCurrentItem || !surfaceHost.Window.window) {
                        return
                    }

                    const window = surfaceHost.Window.window
                    const windowPosition = surfaceHost.mapToItem(window.contentItem, 0, 0)
                    const globalPosition = Qt.point(windowPosition.x + window.x,
                                                    windowPosition.y + window.y)

                    plugin.updatePluginGeometry(Qt.rect(Math.round(windowPosition.x),
                                                        Math.round(windowPosition.y),
                                                        Math.round(width),
                                                        Math.round(height)))
                    plugin.setGlobalPos(Qt.point(Math.round(globalPosition.x),
                                                 Math.round(globalPosition.y)))
                    surfaceItem.fixPosition()
                }

                Component.onCompleted: updateSurfaceGeometry()
                onWidthChanged: geometryUpdateTimer.restart()
                onHeightChanged: geometryUpdateTimer.restart()
                onVisibleChanged: {
                    geometryUpdateTimer.restart()
                    root.updateContentHovered()
                }
                onSurfaceHoveredChanged: root.updateContentHovered()

                Timer {
                    id: geometryUpdateTimer
                    interval: 50
                    repeat: false
                    onTriggered: surfaceHost.updateSurfaceGeometry()
                }

                // A plugin reports its order right after its card surface is
                // created, so the card starts out unordered and has to be
                // sorted into place once the value arrives.
                Connections {
                    target: surfaceHost.plugin
                    ignoreUnknownSignals: true

                    function onCardOrderChanged() {
                        root.modelChanging = true
                        syncTimer.restart()
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        z: 1

        WheelHandler {
            target: null
            enabled: swipeView.count > 1
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

            onWheel: function(wheel) {
                const deltaY = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y
                if (deltaY === 0) {
                    return
                }

                const step = deltaY < 0 ? 1 : -1
                const nextIndex = (swipeView.currentIndex + step + swipeView.count) % swipeView.count
                swipeView.setCurrentIndex(nextIndex)
                wheel.accepted = true
            }
        }
    }

    Connections {
        target: swipeView.contentItem
        ignoreUnknownSignals: true

        function onContentYChanged() {
            surfaceGeometryUpdateTimer.restart()
        }

    }

    Timer {
        id: surfaceGeometryUpdateTimer

        interval: 50
        repeat: false
        onTriggered: {
            const item = swipeView.currentItem
            if (item) {
                item.updateSurfaceGeometry()
            }
        }
    }

    // Card surfaces are not tray items, so their popups are not routed by the
    // tray's per-item handlers and have to be shown here.  Both the tooltip and
    // the menu reuse the shared panel windows, whose WaylandOutputs are set up
    // by the tray applet.
    PanelToolTip {
        id: cardToolTip

        property alias shellSurface: cardToolTipContent.shellSurface

        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y

        ShellSurfaceItemProxy {
            id: cardToolTipContent

            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: cardToolTip.close()
        }
    }

    PanelMenu {
        id: cardMenu

        property alias shellSurface: cardMenuContent.shellSurface

        width: cardMenuContent.width
        height: cardMenuContent.height
        menuX: DockPositioner.x
        menuY: DockPositioner.y

        Item {
            anchors.fill: parent

            ShellSurfaceItemProxy {
                id: cardMenuContent

                anchors.centerIn: parent
                autoClose: true
                onSurfaceDestroyed: cardMenu.close()
            }

            // Hand the final placement back to the plugin, it positions its own
            // menu window relative to it.
            Connections {
                target: cardMenu.menuWindow
                enabled: cardMenu.readyBinding

                function onUpdateGeometryFinished() {
                    if (!cardMenu.shellSurface) {
                        return
                    }

                    cardMenu.shellSurface.updatePluginGeometry(
                        Qt.rect(cardMenu.menuWindow.x, cardMenu.menuWindow.y, 0, 0))
                }
            }
        }
    }

    Connections {
        target: DockCompositor

        function onPopupCreated(popupSurface) {
            const isTooltip = popupSurface.popupType === Dock.TrayPopupTypeTooltip
            const isMenu = popupSurface.popupType === Dock.TrayPopupTypeMenu
            if (!isTooltip && !isMenu) {
                return
            }

            const surfaceId = `${popupSurface.pluginId}::${popupSurface.itemKey}`
            if (!DockCompositor.findSurfaceFromModel(DockCompositor.cardPluginSurfaces, surfaceId)) {
                return
            }

            if (isTooltip) {
                cardToolTip.shellSurface = popupSurface
                // The plugin anchors the tooltip at the horizontal center of its
                // card, DockPanelPositioner turns that into the final placement.
                cardToolTip.DockPanelPositioner.bounding = Qt.binding(function () {
                    return Qt.rect(cardToolTip.shellSurface.x, cardToolTip.shellSurface.y,
                                   cardToolTip.width, cardToolTip.height)
                })
                cardToolTip.open()
                return
            }

            // The menu is anchored at the cursor, and replaces whatever popup is
            // currently open.
            cardToolTip.close()
            cardMenu.shellSurface = popupSurface
            cardMenu.DockPositioner.bounding = Qt.binding(function () {
                return Qt.rect(cardMenu.shellSurface.x, cardMenu.shellSurface.y,
                               cardMenu.width, cardMenu.height)
            })
            Panel.requestClosePopup()
            cardMenu.open()
            // Let the menu surface handle key events, e.g. arrow keys.
            cardMenuContent.takeFocus()
        }
    }

    // The popups belong to the card that is currently on screen, hide them as
    // soon as the whole card area goes away.
    onShouldVisibleChanged: {
        if (!shouldVisible) {
            cardToolTip.close()
            cardMenu.close()
        }
    }
}
