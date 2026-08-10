// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root

    required property var taskManagerItem
    required property var overflow

    readonly property int overflowCount: overflow.popupItems.length
    readonly property var primaryOverflowItem: overflow.popupItems.length > 0 ? overflow.popupItems[0] : null
    readonly property string overflowIconName: primaryOverflowItem && primaryOverflowItem.iconName ? primaryOverflowItem.iconName : ""
    readonly property real popupItemWidth: Math.max(1, overflow.fallbackItemExtent + overflow.spacing)
    readonly property real popupItemHeight: Math.max(1, Panel.rootObject ? Panel.rootObject.dockItemMaxSize : overflow.fallbackItemExtent)
    readonly property real buttonOffset: overflow.visibleItemCount * (overflow.fallbackItemExtent + overflow.spacing)
    readonly property real overflowIconSize: Math.max(12, Math.round(Panel.rootObject.dockItemIconSize))
    readonly property int hoverVerticalSpacing: Math.round(overflowIconSize / 8) + 1
    readonly property int hoverHorizontalSpacing: Math.round(overflowIconSize / 8)
    readonly property int hoverBackgroundWidth: Math.round(overflowIconSize + hoverHorizontalSpacing * 2)
    readonly property int hoverBackgroundHeight: Math.round(overflowIconSize + hoverVerticalSpacing * 2)
    readonly property real dotSize: Math.max(2, Math.round(overflowIconSize * 0.11))
    readonly property real dotSpacing: Math.max(1, Math.round(dotSize * 0.75))
    readonly property color dotColor: D.DTK.themeType === D.ApplicationHelper.DarkType ? "#FFFFFF" : "#000000"
    readonly property int popupColumns: Math.min(6, Math.max(1, overflowCount))
    property point popupAnchorPoint: Qt.point(0, 0)

    visible: overflowCount > 0
    x: root.taskManagerItem.useColumnLayout ? 0 : root.buttonOffset
    y: root.taskManagerItem.useColumnLayout ? root.buttonOffset : 0
    implicitWidth: root.taskManagerItem.useColumnLayout ? root.taskManagerItem.implicitWidth : overflow.fallbackItemExtent
    implicitHeight: root.taskManagerItem.useColumnLayout ? overflow.fallbackItemExtent : root.taskManagerItem.implicitHeight
    width: implicitWidth
    height: implicitHeight

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
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: false
        onClicked: root.togglePopup()
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
        text: qsTr("Show hidden icons")
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
        width: overflowPopupContent.width
        height: overflowPopupContent.height
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

        Control {
            id: overflowPopupContent
            padding: 10
            implicitWidth: overflowGrid.implicitWidth + leftPadding + rightPadding
            implicitHeight: overflowGrid.implicitHeight + topPadding + bottomPadding
            width: implicitWidth
            height: implicitHeight
            background: Item {}

            DropArea {
                anchors.fill: parent
                keys: ["text/x-dde-dock-dnd-appid"]
                onDropped: function(drop) {
                    if (drop.getDataAsString("text/x-dde-dock-dnd-source") === "overflow-popup") {
                        drop.accept(Qt.MoveAction)
                    }
                }
            }

            contentItem: Grid {
                id: overflowGrid
                columns: root.popupColumns
                rowSpacing: Math.max(6, Math.round(root.popupItemWidth * 0.12))
                columnSpacing: rowSpacing

                Repeater {
                    model: root.overflow.popupItems

                    delegate: Item {
                        required property var modelData

                        width: root.popupItemWidth
                        height: root.popupItemHeight

                        AppItem {
                            width: parent.width
                            height: parent.height
                            displayMode: Panel.indicatorStyle
                            colorTheme: Panel.colorTheme
                            active: modelData.active
                            attention: modelData.attention
                            itemId: modelData.itemId
                            name: modelData.name
                            iconName: modelData.iconName
                            menus: modelData.menus
                            windows: modelData.windows
                            groupItems: modelData.groupItems
                            visualIndex: modelData.visualIndex
                            modelIndex: modelData.modelIndex
                            blendOpacity: root.taskManagerItem.blendOpacity
                            title: modelData.title
                            enableTitle: false
                            preservePanelPopupOnContextMenu: true
                            dragEnabled: modelData.docked
                            dragSource: "overflow-popup"
                            restorePositionAfterDrag: true

                            Component.onCompleted: {
                                dropFilesOnItem.connect(root.taskManagerItem.Applet.dropFilesOnItem)
                            }
                            onDragFinished: function(dropAction) {
                                if (dropAction === Qt.IgnoreAction && modelData.docked) {
                                    root.taskManagerItem.Applet.requestUndockByDesktopId(modelData.itemId)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
