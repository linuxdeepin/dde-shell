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
    id: appItem
    required property int displayMode
    required property int colorTheme
    required property bool active
    required property string itemId
    required property string name
    required property string iconName
    required property string menus
    required property list<string> windows
    required property int visualIndex

    signal clickItem(itemId: string)
    signal clickItemMenu(itemId: string, menuId: string)

    visible: !Drag.active // When in dragging, hide app item
    Drag.active: dragHandler.active
    Drag.source: appItem
    Drag.hotSpot.x: icon.width / 2
    Drag.hotSpot.y: icon.height / 2
    Drag.dragType: Drag.Automatic
    Drag.mimeData: { "text/x-dde-dock-dnd-appid": itemId }
    Drag.supportedActions: Qt.MoveAction

    DragHandler {
        id: dragHandler
    }

    // Use TapHandler for tap screen compatibility
    // Using TapHandler also means that we need a complete single tap to trigger menu, not just a press down
    TapHandler {
        id: tapHandler
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        // NOTICE: Setting gesturePolicy is mandatory for exclusive point event grab.
        // Otherwise default passive grab will propagate event to parent
        gesturePolicy: TapHandler.WithinBounds
        onPressedChanged: {
            if (pressed) {
                // Grab icon as image in advance for drag's use
                icon.grabToImage(function(result) {
                    appItem.Drag.imageSource = result.url;
                })
            }
        }
        onTapped: function(eventPoint, button) {
            if (button === Qt.RightButton) {
                contextMenu.open()
            } else {
                appItem.clickItem(itemId)
            }
        }
    }

    AppItemPalette {
        id: itemPalette
        displayMode: appItem.displayMode
        colorTheme: appItem.colorTheme
        active: appItem.active
    }

    Loader {
        id: statusIndicatorLoader
        anchors.centerIn: parent
        property int displayMode: appItem.displayMode
        sourceComponent: (displayMode === Dock.Efficient && windows.length > 0) ? statusIndicator : null
    }

    Loader {
        id: windowIndicatorLoader
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 1
        }
        property int displayMode: appItem.displayMode
        property list<string> windows: appItem.windows
        sourceComponent: (displayMode === Dock.Efficient && windows.length > 1) || (displayMode === Dock.Fashion && windows.length > 0) ? windowIndicator : null
    }

    Component {
        id: statusIndicator
        StatusIndicator {
            palette: itemPalette
            width: icon.width * 1.1
            height: icon.height * 1.1
        }
    }

    Component {
        id: windowIndicator
        WindowIndicator {
            windows: appItem.windows
            displayMode: appItem.displayMode
            palette: itemPalette
        }
    }

    LP.Menu {
        id: contextMenu
        Instantiator {
            id: menuItemInstantiator
            model: JSON.parse(menus)
            delegate: LP.MenuItem {
                text: modelData.name
                onTriggered: {
                    console.log(itemId, modelData.id)
                    appItem.clickItemMenu(itemId, modelData.id)
                }
            }
            onObjectAdded: (index, object) => contextMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => contextMenu.removeItem(object)
        }
    }

    D.DciIcon {
        id: icon
        name: iconName
        sourceSize: Qt.size(parent.width * 0.6, parent.height * 0.6)
        anchors.centerIn: parent
    }
}
