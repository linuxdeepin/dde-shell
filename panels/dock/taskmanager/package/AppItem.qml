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
    signal dragFinished()

    Drag.active: mouseArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: icon.width / 2
    Drag.hotSpot.y: icon.height / 2
    Drag.dragType: Drag.Automatic
    Drag.mimeData: { "text/x-dde-dock-dnd-appid": itemId }

    property bool useColumnLayout: Panel.position % 2
    property int statusIndicatorSize: useColumnLayout ? root.width * 0.72 : root.height * 0.72
    property real iconScale: Panel.rootObject.dockItemMaxSize * 9 / 14 / Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE

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
            backgroundColor: D.DTK.palette.highlight
        }

        StatusIndicator {
            id: statusIndicator
            palette: itemPalette
            width: root.statusIndicatorSize
            height: root.statusIndicatorSize
            anchors.centerIn: icon
            visible: root.displayMode === Dock.Efficient && root.windows.length > 0
        }

        WindowIndicator {
            id: windowIndicator
            dotWidth: root.useColumnLayout  ? Math.max(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE * iconScale / 16, 2) : Math.max(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE * iconScale / 3, 2) 
            dotHeight: root.useColumnLayout ? Math.max(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE * iconScale / 3, 2) : Math.max(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE * iconScale / 16, 2)
            windows: root.windows
            displayMode: root.displayMode
            useColumnLayout: root.useColumnLayout
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
                let fixedDistance = 2

                switch(Panel.position) {
                case Dock.Top: {
                    windowIndicator.anchors.horizontalCenter = parent.horizontalCenter
                    windowIndicator.anchors.top = parent.top
                    windowIndicator.anchors.topMargin = fixedDistance
                    return
                }
                case Dock.Bottom: {
                    windowIndicator.anchors.horizontalCenter = parent.horizontalCenter
                    windowIndicator.anchors.bottom = parent.bottom
                    windowIndicator.anchors.bottomMargin = fixedDistance
                    return
                }
                case Dock.Left: {
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.left = parent.left
                    windowIndicator.anchors.leftMargin = fixedDistance
                    return
                }
                case Dock.Right:{
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.right = parent.right
                    windowIndicator.anchors.rightMargin = fixedDistance
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

        Loader {
            id: contextMenuLoader
            active: false
            sourceComponent: LP.Menu {
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
        }

        D.DciIcon {
            id: icon
            name: root.iconName
            sourceSize: Qt.size(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE, Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE)
            anchors.centerIn: parent
            scale: iconScale
            BeatAnimation {
                id: beatAnimation
                target: icon
                baseScale: iconScale
                loops: Animation.Infinite
                running: root.attention
            }

            LaunchAnimation {
                id: launchAnimation
                launchSpace: {
                    switch (Panel.position) {
                    case Dock.Top:
                    case Dock.Bottom:
                    // todo: use icon.height * iconScale is not good
                    return (root.height - icon.height * iconScale) / 2
                    case Dock.Left:
                    case Dock.Right:
                        return (root.width - icon.width * iconScale) / 2
                    }
                }

                direction: {
                    switch (Panel.position) {
                    case Dock.Top:
                        return LaunchAnimation.Direction.Down
                    case Dock.Bottom:
                        return LaunchAnimation.Direction.Up
                    case Dock.Left:
                        return LaunchAnimation.Direction.Right
                    case Dock.Right:
                        return LaunchAnimation.Direction.Left
                    }
                }
                target: icon
                loops: 1
                running: false
            }

            Connections {
                target: Panel.rootObject
                function onPressedAndDragging(isDragging) {
                    if (isDragging) {
                        beatAnimation.stop()
                        icon.scale = Qt.binding(function() {
                            return root.iconScale
                        })
                    } else {
                        beatAnimation.running = Qt.binding(function() {
                            return root.attention
                        })
                    }
                }
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

    Timer {
        id: previewTimer
        interval: 500
        running: false
        repeat: false
        property int xOffset: 0
        property int yOffset: 0
        onTriggered: {
            if (root.windows.length != 0) {
                taskmanager.Applet.showItemPreview(root.itemId, Panel.rootObject, xOffset, yOffset, Panel.position)
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        drag.target: root
        drag.onActiveChanged: {
            if (!drag.active)
                root.dragFinished()
        }

        onPressed: function (mouse) {
            if (mouse.button === Qt.LeftButton) {
                icon.grabToImage(function(result) {
                    root.Drag.imageSource = result.url;
                })
            }
            toolTip.close()
            closeItemPreview()
        }
        onClicked: function (mouse) {
            if (mouse.button === Qt.RightButton) {
                contextMenuLoader.active = true
                MenuHelper.openMenu(contextMenuLoader.item)
            } else {
                if (root.windows.length === 0) {
                    launchAnimation.start()
                }
                root.clickItem(root.itemId, "")
            }
        }

        onEntered: {
            if (windows.length === 0) {
                toolTipShowTimer.start()
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
            previewTimer.xOffset = xOffset
            previewTimer.yOffset = yOffset
            previewTimer.start()
        }

        onExited: {
            if (toolTipShowTimer.running) {
                toolTipShowTimer.stop()
            }

            if (windows.length === 0) {
                toolTip.close()
                return
            }
            closeItemPreview()
        }

        PanelToolTip {
            id: toolTip
            text: root.name
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

        function closeItemPreview() {
            if (previewTimer.running) {
                previewTimer.stop()
            } else {
                taskmanager.Applet.hideItemPreview()
            }
        }
    }

    onWindowsChanged: {
        updateWindowIconGeometryTimer.start()
    }

    onIconGlobalPointChanged: {
        updateWindowIconGeometryTimer.start()
    }
}
