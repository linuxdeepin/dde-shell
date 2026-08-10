// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

PanelPopup {
    id: root

    required property string groupName
    required property var groupItems
    required property int displayMode

    readonly property int itemCount: root.groupItems ? root.groupItems.length : 0
    readonly property int columnCount: root.itemCount > 12 ? 5 : 4
    readonly property int rowCount: Math.max(1, Math.ceil(root.itemCount / root.columnCount))
    readonly property int visibleRowCount: Math.min(3, root.rowCount)

    readonly property int itemWidth: 96
    readonly property int itemHeight: 104
    readonly property int itemIconSize: 48
    readonly property int itemSpacing: 8
    readonly property int horizontalPadding: 5
    readonly property int titleTopPadding: 12
    readonly property int titleHeight: 24
    readonly property int titleBottomSpacing: 8
    readonly property int bottomPadding: 22
    readonly property int gridWidth: root.columnCount * root.itemWidth
                                     + (root.columnCount - 1) * root.itemSpacing
    readonly property int gridHeight: root.rowCount * root.itemHeight
                                      + (root.rowCount - 1) * root.itemSpacing
    readonly property int visibleGridHeight: root.visibleRowCount * root.itemHeight
                                             + (root.visibleRowCount - 1) * root.itemSpacing

    property point anchorPoint: Qt.point(0, 0)

    signal launchApplicationRequested(string desktopId)

    width: root.gridWidth + root.horizontalPadding * 2
    height: root.titleTopPadding + root.titleHeight + root.titleBottomSpacing
            + root.visibleGridHeight + root.bottomPadding
    popupX: DockPanelPositioner.x
    popupY: DockPanelPositioner.y
    closeOnInactive: MenuHelper.activeMenu === null && !Panel.contextDragging

    function openAt(point) {
        root.anchorPoint = point
        popupFlickable.contentY = 0
        root.open()
    }

    Component.onCompleted: {
        DockPanelPositioner.bounding = Qt.binding(function() {
            return Qt.rect(root.anchorPoint.x, root.anchorPoint.y,
                           root.width, root.height)
        })
    }

    Control {
        id: popupContent

        anchors.fill: parent
        leftPadding: root.horizontalPadding
        rightPadding: root.horizontalPadding
        topPadding: root.titleTopPadding + root.titleHeight + root.titleBottomSpacing
        bottomPadding: root.bottomPadding
        background: Item {}

        Text {
            id: folderTitle

            anchors.top: parent.top
            anchors.topMargin: root.titleTopPadding
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: root.horizontalPadding + 8
            anchors.rightMargin: root.horizontalPadding + 8
            height: root.titleHeight
            text: root.groupName
            textFormat: Text.PlainText
            elide: Text.ElideRight
            maximumLineCount: 1
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: D.DTK.palette.windowText
            font: D.DTK.fontManager.t6
        }

        contentItem: Flickable {
            id: popupFlickable

            contentWidth: width
            contentHeight: root.gridHeight
            clip: true
            interactive: root.rowCount > root.visibleRowCount
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            pixelAligned: true

            Grid {
                id: popupGrid

                width: popupFlickable.width
                height: root.gridHeight
                columns: root.columnCount
                rowSpacing: root.itemSpacing
                columnSpacing: root.itemSpacing

                Repeater {
                    id: groupRepeater
                    model: root.groupItems

                    delegate: Item {
                        id: groupAppItem

                        required property var modelData

                        width: root.itemWidth
                        height: root.itemHeight
                        clip: true

                        Item {
                            id: groupAppContent

                            anchors.fill: parent

                            AppItemBackground {
                                id: groupAppHoverBackground

                                anchors.fill: parent
                                radius: 8
                                enabled: false
                                isActive: false
                                windowCount: 0
                                displayMode: root.displayMode
                                D.ColorSelector.hovered: groupAppHoverHandler.hovered
                                D.ColorSelector.pressed: groupAppMouseArea.pressed
                            }

                            D.DciIcon {
                                id: groupAppIcon

                                width: root.itemIconSize
                                height: width
                                anchors.top: parent.top
                                anchors.topMargin: 7
                                anchors.horizontalCenter: parent.horizontalCenter
                                name: modelData.iconName
                                sourceSize: Qt.size(width, height)
                            }

                            Text {
                                id: groupAppTitle

                                anchors.top: groupAppIcon.bottom
                                anchors.topMargin: 4
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 2
                                anchors.rightMargin: 2
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignTop
                                text: modelData.name
                                color: D.DTK.palette.windowText
                                font: D.DTK.fontManager.t9
                            }

                            HoverHandler {
                                id: groupAppHoverHandler
                            }

                            MouseArea {
                                id: groupAppMouseArea

                                anchors.fill: parent
                                onClicked: {
                                    root.launchApplicationRequested(modelData.desktopId)
                                    root.close()
                                }
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                parent: popupContent
                anchors.top: popupFlickable.top
                anchors.right: popupContent.right
                anchors.bottom: popupFlickable.bottom
                anchors.rightMargin: 1
                width: 4
                policy: root.rowCount > root.visibleRowCount
                        ? ScrollBar.AsNeeded
                        : ScrollBar.AlwaysOff
            }
        }
    }
}
