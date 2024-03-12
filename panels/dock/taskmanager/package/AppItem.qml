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

    property int statusIndicatorSize: root.width * 0.9
    property int iconSize: root.width * 0.6

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
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 1
            }
            windows: root.windows
            displayMode: root.displayMode
            palette: itemPalette
            visible: (root.displayMode === Dock.Efficient && root.windows.length > 1) || (root.displayMode === Dock.Fashion && root.windows.length > 0)
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
            if (windows.length === 0) return
            var itemPos = root.mapToItem(null, 0, 0)
            if (Panel.position % 2 === 0) {
                itemPos.x += (root.width / 2)
                itemPos.y += (Panel.position == 2 ? -10 : Panel.dockSize + 10)
            } else {
                itemPos.x += (Panel.position == 1 ? -10 : Panel.dockSize + 10)
                itemPos.y += (root.height / 2)
            }
            taskmanager.Applet.showItemPreview(root.itemId, Panel.rootObject, itemPos.x, itemPos.y, Panel.position)
        }

        onExited: {
            if (windows.length === 0) return
            taskmanager.Applet.hideItemPreview()
        }
    }
}
