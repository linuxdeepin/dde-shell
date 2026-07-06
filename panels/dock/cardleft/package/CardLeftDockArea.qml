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

    readonly property color primaryTextColor: Panel.colorTheme === Dock.Dark
        ? Qt.rgba(1, 1, 1, 0.96)
        : Qt.rgba(0, 0, 0, 0.92)

    dockOrder: 5
    shouldVisible: fashionMode && pageCount > 0
    readonly property int pageCount: DockCompositor.cardPluginSurfaces.count
    property bool contentHovered: false
    readonly property bool effectiveHovered: rootHoverHandler.hovered || contentHovered

    visible: shouldVisible
    implicitWidth: adaptiveCardLeftWidth
    implicitHeight: dockSize
    clip: true

    function updateContentHovered() {
        const item = swipeView.currentItem
        contentHovered = !!item && item.surfaceHovered
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
                color: root.primaryTextColor
                opacity: index === pageIndicator.currentIndex ? 1 : 0.35
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

        Repeater {
            model: DockCompositor.cardPluginSurfaces

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
}
