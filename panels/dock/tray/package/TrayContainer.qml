// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
## Design Overview

The tray area follows the following design:
.___.
| O |
.___.
  v
_________________________________________
 [.] (@) (@) [>] (%) (%) [=] [%] [12:34]

[.]: Tray popup action (to show stashed icons)
(@): Collapsable tray icons
[>]: Collapse action icon
(%): Pinned tray icons
[=]: Quick settings panel action icon
[%]: Fixed tray plugins
[12:34]: Date-time plugin (one of the fixed tray plugins)

## Implementation Overview

Dock plugins are categorized as 4 types:

1. Stashed Trays
2. Collapsable Trays
3. Pinned Trays (can still drag to re-arrange or put to stased or collapsable area)
4. Fixed Trays (cannot drag, or declared as fixed)

We'll need a class (TraySortOrderModel) dedicated to load/save tray icon sort order.
Such model store the positions of all known tray items. The sort order is reflected
as a property instead of the item index of the model, and is intended to use together
with a/multple proxy model(s).

class TraySortOrderModel {
property:
    collapsed: bool, get, set
roles:
    pluginId: string
    visiblity: bool // can be used for filtering
    sectionType: enum {
        tray-action // sort not involved by section type
        stashed
        collapsable
        pinned
        fixed
    }
    visualIndex: int // global sort order, calculated by the model from C++ side, can be duplicated, cannot have gap number
    delegateType: enum { // for DelegateChooser
        legacy-tray-plugin
        action-show-stash
        action-toggle-collapse
        action-toggle-quick-settings
        dummy // test-only, id as icon name
    }
function:
    // DnD related
    void stageChange() // update visual index, but not actual sort order (e.g. when dragging)
    void commitChange() // save change on disk, also clear staged cache
    void restoreChange() // restore staged changes (e.g. when cancelling a drag)
}
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1 as LP
import org.deepin.ds.dock.tray 1.0 as DDT

Item {
    id: root

    required property DDT.TraySortOrderModel model

    property string color: "red"
    property bool collapsed: false
    property bool isHorizontal: true

    readonly property int itemSize: 16
    readonly property int itemSpacing: 10

    implicitWidth: width
    width: isHorizontal ? (DDT.TraySortOrderModel.visualItemCount * (itemSize + itemSpacing) - itemSpacing) : 16
    implicitHeight: height
    height: !isHorizontal ? (DDT.TraySortOrderModel.visualItemCount * (itemSize + itemSpacing) - itemSpacing) : 16

    Behavior on width {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    // Delegates
    TrayItemDelegateChooser {
        id: trayItemDelegateChooser
        isHorizontal: root.isHorizontal
        collapsed: root.collapsed
    }

    // debug
    Rectangle {
        color: root.color
        anchors.fill: parent
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/x-dde-shell-tray-dnd-surfaceId"]
        onEntered: function (dragEvent) {
            console.log(dragEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId"))
        }
        onPositionChanged: function (dragEvent) {
            let surfaceId = dragEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId")
            let pos = root.isHorizontal ? drag.x : drag.y
            let currentItemIndex = pos / (root.itemSize + root.itemSpacing)
            let currentPosMapToItem = pos % (root.itemSize + root.itemSpacing)
            let isBefore = currentPosMapToItem < root.itemSize / 2
            console.log("dragging", surfaceId, Math.floor(currentItemIndex), currentPosMapToItem, isBefore)
            // DDT.TraySortOrderModel.dropToDockTray(surfaceId, Math.floor(currentItemIndex), isBefore);
        }
        onDropped: function (dropEvent) {
            let surfaceId = dropEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId")
            let pos = root.isHorizontal ? drag.x : drag.y
            let currentItemIndex = pos / (root.itemSize + root.itemSpacing)
            let currentPosMapToItem = pos % (root.itemSize + root.itemSpacing)
            let isBefore = currentPosMapToItem < root.itemSize / 2
            console.log("dropped", currentItemIndex, currentPosMapToItem, isBefore)
            DDT.TraySortOrderModel.dropToDockTray(surfaceId, Math.floor(currentItemIndex), isBefore);
        }
    }

    // Tray items
    Repeater {
        anchors.fill: parent
        model: root.model
        delegate: trayItemDelegateChooser
    }
}
