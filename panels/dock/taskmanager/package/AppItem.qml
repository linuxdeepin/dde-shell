// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D
import Qt.labs.platform 1.1 as LP

Item {
    id: root
    required property int displayMode
    required property int colorTheme
    required property bool active
    required property bool attention
    required property string itemId
    required property string name
    required property string iconName
    required property string menus
    required property list<string> windows
    required property int visualIndex

    signal clickItem(itemId: string, menuId: string)

    Drag.active: mouseArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: icon.width / 2
    Drag.hotSpot.y: icon.height / 2
    Drag.dragType: Drag.Automatic
    Drag.mimeData: { "text/x-dde-dock-dnd-appid": itemId }

    property int statusIndicatorSize: root.width * 0.8
    property int iconSize: Panel.rootObject.itemIconSizeBase * 0.64

    property var iconGlobalPoint: {
        var a = icon
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x, y)
    }

    Item {
        anchors.fill: parent
        id: appItem
        visible: !root.Drag.active // When in dragging, hide app item
        AppItemPalette {
            id: itemPalette
            displayMode: root.displayMode
            colorTheme: root.colorTheme
            active: root.active
            backgroundColor: "orange"
        }

        StatusIndicator {
            id: statusIndicator
            palette: itemPalette
            width: root.statusIndicatorSize
            height: root.statusIndicatorSize
            anchors.centerIn: parent
            visible: root.displayMode === Dock.Efficient && root.windows.length > 0
        }

        WindowIndicator {
            id: windowIndicator
            dotWidth: Panel.position % 2 ? Math.max(root.width * 0.04, 2) : Math.max(root.width * 0.18, 8) 
            dotHeight: Panel.position % 2 ? Math.max(root.width * 0.18, 8) : Math.max(root.width * 0.04, 2)
            windows: root.windows
            displayMode: root.displayMode
            useColumnLayout: Panel.position % 2
            palette: itemPalette
            visible: (root.displayMode === Dock.Efficient && root.windows.length > 1) || (root.displayMode === Dock.Fashion && root.windows.length > 0)

            function updateIndicatorAnchors() {
                windowIndicator.anchors.top = undefined
                windowIndicator.anchors.topMargin = 0
                windowIndicator.anchors.bottom = undefined
                windowIndicator.anchors.bottomMargin = 0
                windowIndicator.anchors.left = undefined
                windowIndicator.anchors.leftMargin = 0
                windowIndicator.anchors.right = undefined
                windowIndicator.anchors.rightMargin = 0
                windowIndicator.anchors.horizontalCenter = undefined
                windowIndicator.anchors.verticalCenter = undefined

                switch(Panel.position) {
                case Dock.Top: {
                    windowIndicator.anchors.horizontalCenter = parent.horizontalCenter
                    windowIndicator.anchors.top = parent.top
                    windowIndicator.anchors.topMargin = Qt.binding(function() {
                        return Math.floor(root.width * 0.1) - 1
                    })
                    return
                }
                case Dock.Bottom: {
                    windowIndicator.anchors.horizontalCenter = parent.horizontalCenter
                    windowIndicator.anchors.bottom = parent.bottom
                    windowIndicator.anchors.bottomMargin = Qt.binding(function() {
                        return Math.floor(root.width * 0.1) - 1
                    })
                    return
                }
                case Dock.Left: {
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.left = parent.left
                    windowIndicator.anchors.leftMargin = Qt.binding(function() {
                        return Math.floor(root.width * 0.1) - 1
                    })
                    return
                }
                case Dock.Right:{
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.right = parent.right
                    windowIndicator.anchors.rightMargin = Qt.binding(function() {
                        return Math.floor(root.width * 0.1) - 1
                    })
                    return
                }
                }
            }

            Component.onCompleted: {
                windowIndicator.updateIndicatorAnchors()
            }
        }

        Connections {
            function onPositionChanged() {
                windowIndicator.updateIndicatorAnchors()
                updateWindowIconGeometryTimer.start()
            }
            target: Panel
        }

        LP.Menu {
            id: contextMenu
            Instantiator {
                id: menuItemInstantiator
                model: JSON.parse(menus)
                delegate: LP.MenuItem {
                    text: modelData.name
                    onTriggered: {
                        root.clickItem(root.itemId, modelData.id)
                    }
                }
                onObjectAdded: (index, object) => contextMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => contextMenu.removeItem(object)
            }
        }

        D.DciIcon {
            id: icon
            name: root.iconName
            sourceSize: Qt.size(iconSize, iconSize)
            anchors.centerIn: parent
            scale: Panel.rootObject.itemScale
            BeatAnimation {
                target: icon
                loops: Animation.Infinite
                running: root.attention
            }

            LaunchAnimation {
                id: launchAnimation
                target: icon
                loops: 1
                running: false
            }
        }
    }

    Timer {
        id: updateWindowIconGeometryTimer
        interval: 500
        running: false
        repeat: false
        onTriggered: {
            taskmanager.Applet.setAppItemWindowIconGeometry(root.itemId, Panel.rootObject, iconGlobalPoint.x, iconGlobalPoint.y,
                iconGlobalPoint.x + icon.width, iconGlobalPoint.y + icon.height)
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        drag.target: root
        onPressed: function (mouse) {
            if (mouse.button === Qt.LeftButton) {
                icon.grabToImage(function(result) {
                    root.Drag.imageSource = result.url;
                })
            }
            toolTip.close()
        }
        onClicked: function (mouse) {
            if (mouse.button === Qt.RightButton) {
                MenuHelper.openMenu(contextMenu)
            } else {
                if (root.windows.length === 0) {
                    launchAnimation.start()
                }
                root.clickItem(root.itemId, "")
            }
        }

        onEntered: {
            if (windows.length === 0) {
                var point = root.mapToItem(null, root.width / 2, 0)
                toolTip.toolTipX = point.x
                toolTip.toolTipY = point.y
                toolTip.open()
                return
            }

            var itemPos = root.mapToItem(null, 0, 0)
            let xOffset, yOffset, interval = 10
            if (Panel.position % 2 === 0) {
                xOffset = itemPos.x + (root.width / 2)
                yOffset = (Panel.position == 2 ? -interval : interval + Panel.dockSize)
            } else {
                xOffset = (Panel.position == 1 ? -interval : interval + Panel.dockSize)
                yOffset = itemPos.y + (root.height / 2)
            }
            taskmanager.Applet.showItemPreview(root.itemId, Panel.rootObject, xOffset, yOffset, Panel.position)
        }

        onExited: {
            if (windows.length === 0) {
                toolTip.close()
                return
            }
            taskmanager.Applet.hideItemPreview()
        }

        PanelToolTip {
            id: toolTip
            text: root.name
        }
    }

    onWindowsChanged: {
        updateWindowIconGeometryTimer.start()
    }

    onIconGlobalPointChanged: {
        updateWindowIconGeometryTimer.start()
    }
}
