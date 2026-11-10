// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root

    required property var taskManagerItem
    required property var overflow

    signal dragFinished(int dropAction, string itemId, bool canUndock)

    readonly property int overflowCount: overflow.popupCount
    readonly property var primaryOverflowItem: overflow.popupCount > 0 ? overflow.primaryItem : null
    readonly property string overflowIconName: primaryOverflowItem && primaryOverflowItem.iconName ? primaryOverflowItem.iconName : ""
    readonly property real popupItemWidth: overflow.fallbackItemExtent
    readonly property real popupItemHeight: overflow.fallbackItemExtent
    readonly property real buttonOffset: overflow.buttonOffset
    readonly property real overflowIconSize: Math.min(overflow.buttonExtent, Math.max(12, Math.round(Panel.rootObject.dockItemIconSize)))
    readonly property int hoverVerticalSpacing: Math.round(overflowIconSize / 8) + 1
    readonly property int hoverHorizontalSpacing: Math.round(overflowIconSize / 8)
    readonly property int hoverBackgroundWidth: Math.round(overflowIconSize + hoverHorizontalSpacing * 2)
    readonly property int hoverBackgroundHeight: Math.round(overflowIconSize + hoverVerticalSpacing * 2)
    readonly property real dotSize: Math.max(2, Math.round(overflowIconSize * 0.11))
    readonly property real dotSpacing: Math.max(1, Math.round(dotSize * 0.75))
    readonly property color dotColor: D.DTK.themeType === D.ApplicationHelper.DarkType ? "#FFFFFF" : "#000000"
    readonly property color separatorColor: D.DTK.themeType === D.ApplicationHelper.DarkType
        ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
    readonly property real gridSpacing: overflow.spacing
    readonly property int popupCellWidth: Math.ceil(popupItemWidth + gridSpacing)
    readonly property int popupCellHeight: Math.ceil(popupItemHeight
        + Math.max(10, 2 * (Math.round(popupItemHeight / 8) + 2)))
    readonly property real popupAvailableWidth: Math.max(popupCellWidth + 20,
        Screen.width - 40 - (root.taskManagerItem.useColumnLayout ? Panel.rootObject.dockSize : 0))
    readonly property real popupAvailableHeight: Math.max(popupCellHeight + 20,
        Screen.height - 40 - (root.taskManagerItem.useColumnLayout ? 0 : Panel.rootObject.dockSize))
    readonly property int popupColumns: Math.min(6, Math.max(1, overflowCount),
        Math.max(1, Math.floor((popupAvailableWidth - 20) / popupCellWidth)))
    property point popupAnchorPoint: Qt.point(0, 0)

    visible: overflowCount > 0 && overflow.buttonExtent > 0
    x: root.taskManagerItem.useColumnLayout ? 0 : root.buttonOffset
    y: root.taskManagerItem.useColumnLayout ? root.buttonOffset : 0
    implicitWidth: root.taskManagerItem.useColumnLayout ? root.taskManagerItem.implicitWidth : overflow.buttonExtent
    implicitHeight: root.taskManagerItem.useColumnLayout ? overflow.buttonExtent : root.taskManagerItem.implicitHeight
    width: implicitWidth
    height: implicitHeight

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Overflow area")

    // Leave a full application gap between each outer line and its shortcut.
    // A vertical dock uses horizontal lines before and after the group.
    Repeater {
        model: 2

        Rectangle {
            required property int index
            visible: overflow.separatorPadding >= 1
            readonly property real axisPosition: index === 0
                ? -(overflow.showRecent ? overflow.fallbackItemExtent + overflow.spacing : 0) - overflow.separatorPadding
                : overflow.buttonExtent + overflow.separatorPadding - 1

            width: root.taskManagerItem.useColumnLayout ? Panel.rootObject.dockItemIconSize : 1
            height: root.taskManagerItem.useColumnLayout ? 1 : Panel.rootObject.dockItemIconSize
            x: root.taskManagerItem.useColumnLayout ? (root.width - width) / 2 : Math.round(axisPosition)
            y: root.taskManagerItem.useColumnLayout ? Math.round(axisPosition) : (root.height - height) / 2
            color: root.separatorColor
            Accessible.ignored: true
        }
    }

    // The recent shortcut precedes the overflow entry in both dock orientations.
    Loader {
        active: root.visible && root.overflow.showRecent && !!root.overflow.recentItem.itemId
        x: root.taskManagerItem.useColumnLayout ? 0 : -(overflow.fallbackItemExtent + overflow.spacing)
        y: root.taskManagerItem.useColumnLayout ? -(overflow.fallbackItemExtent + overflow.spacing) : 0
        width: root.width
        height: root.height
        sourceComponent: AppItem {
            readonly property var entry: root.overflow.recentItem
            displayMode: Panel.indicatorStyle
            colorTheme: Panel.colorTheme
            active: entry.active
            attention: entry.attention
            itemId: entry.itemId
            name: entry.name
            iconName: entry.iconName
            folderType: entry.folderType
            applicationFolderContents: entry.applicationFolderContents
            folderPresentation: entry.folderPresentation
            menus: entry.menus
            windows: entry.windows
            visualIndex: entry.visualIndex
            modelIndex: entry.modelIndex
            title: entry.title
            blendOpacity: root.taskManagerItem.blendOpacity
            dragEnabled: false
            Component.onCompleted: dropFilesOnItem.connect(root.taskManagerItem.Applet.dropFilesOnItem)
        }
    }

    onOverflowCountChanged: {
        if (overflowCount === 0)
            overflowPopup.close()
    }

    Connections {
        target: Panel
        function onPositionChanged() { overflowPopup.close() }
    }

    function togglePopup() {
        toolTipShowTimer.stop()
        toolTip.close()

        if (root.overflowCount <= 0) {
            return
        }

        if (overflowPopup.popupVisible) {
            overflowPopup.close()
            return
        }

        Panel.requestClosePopup()
        popupAnchorPoint = root.mapToItem(null, root.width / 2, root.height / 2)
        popupContentLoader.active = true
        overflowPopup.open()
    }

    AppItemBackground {
        width: root.hoverBackgroundWidth
        height: root.hoverBackgroundHeight
        radius: height / 5
        anchors.centerIn: parent
        enabled: false
        displayMode: Panel.indicatorStyle
        windowCount: 0
        isActive: false
        D.ColorSelector.hovered: overflowHoverHandler.hovered || overflowPopup.popupVisible
        D.ColorSelector.pressed: overflowMouseArea.pressed
    }

    Item {
        id: overflowIndicator
        anchors.centerIn: parent
        visible: root.primaryOverflowItem !== null
        width: root.overflowIconSize
        height: root.overflowIconSize

        D.DciIcon {
            anchors.fill: parent
            name: root.overflowIconName
            sourceSize: Qt.size(width, height)
            retainWhileLoading: true
            smooth: false
            opacity: 0.4
        }
        Row {
            spacing: root.dotSpacing
            anchors.centerIn: parent

            Repeater {
                model: 3

                Rectangle {
                    width: root.dotSize
                    height: width
                    radius: width / 2
                    color: root.dotColor
                }
            }
        }
    }

    MouseArea {
        id: overflowMouseArea
        anchors.fill: parent
        // Consume right clicks without opening the popup or the dock's menu.
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: false
        onClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton)
                root.togglePopup()
        }
    }

    HoverHandler {
        id: overflowHoverHandler
        onHoveredChanged: {
            if (hovered && !overflowPopup.popupVisible) {
                toolTipShowTimer.start()
            } else {
                toolTipShowTimer.stop()
                toolTip.close()
            }
        }
    }

    PanelToolTip {
        id: toolTip
        text: qsTr("Overflow area")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    Timer {
        id: toolTipShowTimer
        interval: 50
        onTriggered: {
            var point = root.mapToItem(null, root.width / 2, root.height / 2)
            toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
            toolTip.open()
        }
    }

    PanelPopup {
        id: overflowPopup
        width: popupContentLoader.item ? popupContentLoader.item.implicitWidth : 0
        height: popupContentLoader.item ? popupContentLoader.item.implicitHeight : 0
        contentReady: popupContentLoader.status === Loader.Ready
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y
        closeOnInactive: MenuHelper.activeMenu === null && !Panel.contextDragging

        Component.onCompleted: {
            DockPanelPositioner.bounding = Qt.binding(function () {
                return Qt.rect(root.popupAnchorPoint.x,
                               root.popupAnchorPoint.y,
                               overflowPopup.width,
                               overflowPopup.height)
            })
        }

        // Create the grid on first use and retain it across close/open and native
        // drags. GridView only instantiates the visible rows (plus its cache).
        Loader {
            id: popupContentLoader
            active: false
            width: overflowPopup.width
            height: overflowPopup.height
            sourceComponent: Control {
                id: overflowPopupContent
                padding: 10
                implicitWidth: root.popupColumns * overflowGrid.cellWidth + leftPadding + rightPadding
                implicitHeight: Math.min(root.popupAvailableHeight,
                    Math.ceil(root.overflowCount / root.popupColumns) * overflowGrid.cellHeight
                    + topPadding + bottomPadding)
                background: Item {}

                DropArea {
                    anchors.fill: parent
                    keys: ["text/x-dde-dock-dnd-appid", "text/x-dde-dock-dnd-folder"]
                    onDropped: function(drop) {
                        if (drop.getDataAsString("text/x-dde-dock-dnd-source") === "overflow-popup")
                            drop.accept(Qt.MoveAction)
                    }
                }

                contentItem: GridView {
                    id: overflowGrid
                    cellWidth: root.popupCellWidth
                    cellHeight: root.popupCellHeight
                    interactive: contentHeight > height && !Panel.contextDragging
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    model: root.overflow.popupModel

                    delegate: Item {
                        id: popupDelegate
                        required property int index
                        required property var entry
                        width: overflowGrid.cellWidth
                        height: overflowGrid.cellHeight
                        GridView.delayRemove: popupApp.Drag.active

                        AppItem {
                            id: popupApp
                            x: (popupDelegate.width - width) / 2
                            y: (popupDelegate.height - height) / 2
                            width: root.popupItemWidth
                            height: root.popupItemHeight
                            displayMode: Panel.indicatorStyle
                            colorTheme: Panel.colorTheme
                            active: popupDelegate.entry.active
                            attention: popupDelegate.entry.attention
                            itemId: popupDelegate.entry.itemId
                            name: popupDelegate.entry.name
                            iconName: popupDelegate.entry.iconName
                            folderType: popupDelegate.entry.folderType
                            applicationFolderContents: popupDelegate.entry.applicationFolderContents
                            folderPresentation: popupDelegate.entry.folderPresentation
                            menus: popupDelegate.entry.menus
                            windows: popupDelegate.entry.windows
                            visualIndex: popupDelegate.entry.visualIndex
                            modelIndex: popupDelegate.entry.modelIndex
                            blendOpacity: root.taskManagerItem.blendOpacity
                            title: popupDelegate.entry.title
                            enableTitle: false
                            preservePanelPopupOnContextMenu: true
                            previewAnchor: overflowPopupContent
                            updateIconGeometry: false
                            dragEnabled: popupDelegate.entry.docked && windows.length === 0
                            dragSource: "overflow-popup"
                            restorePositionAfterDrag: true
                            Drag.onDragStarted: {
                                // Views retain their current item outside the viewport.
                                overflowGrid.currentIndex = popupDelegate.index
                                root.overflow.beginDrag()
                            }
                            onDropFilesOnItem: function(itemId, files) {
                                root.taskManagerItem.Applet.dropFilesOnItem(itemId, files)
                            }
                            onDragFinished: function(dropAction) {
                                root.dragFinished(dropAction, itemId, dragEnabled)
                            }
                        }
                    }
                }
            }
        }
    }
}
