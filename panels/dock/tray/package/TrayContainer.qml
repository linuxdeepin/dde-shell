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

    readonly property int itemVisualSize: DDT.TrayItemPositionManager.itemVisualSize.width
    readonly property int itemSpacing: DDT.TrayItemPositionManager.itemSpacing
    readonly property int itemPadding: DDT.TrayItemPositionManager.itemPadding

    property int trayHeight: 50
    property size containerSize: DDT.TrayItemPositionManager.visualSize
    property bool isDragging: DDT.TraySortOrderModel.actionsAlwaysVisible
    property bool animationEnable: false
    // visiualIndex default value is -1
    property int dropHoverIndex: -1
    required property var surfaceAcceptor
    readonly property bool isDropping: dropArea.containsDrag

    // 启动时关闭动画，10s 后再启用
    Timer {
        id: animationEnableTimer
        interval: 10000
        repeat: false
        onTriggered: {
            animationEnable = true
        }
    }

    implicitWidth: isHorizontal ? trayList.contentItem.childrenRect.width : DDT.TrayItemPositionManager.dockHeight
    implicitHeight: isHorizontal ? DDT.TrayItemPositionManager.dockHeight : trayList.contentItem.childrenRect.height

    Behavior on width {
        enabled: animationEnable
        NumberAnimation { duration: 200; easing.type: collapsed || !DDT.TraySortOrderModel.isCollapsing ? Easing.OutQuad : Easing.InQuad }
    }

    Behavior on height {
        enabled: animationEnable
        NumberAnimation { duration: 200; easing.type: collapsed || !DDT.TraySortOrderModel.isCollapsing ? Easing.OutQuad : Easing.InQuad }
    }

    // debug
    Rectangle {
        color: root.color
        anchors.fill: parent
    }

    // Tray items
    ListView {
        id: trayList
        anchors.fill: parent
        interactive: false
        orientation: root.isHorizontal ? Qt.Horizontal : Qt.Vertical
        spacing: root.itemSpacing
        model: DelegateModel {
            id: visualModel

            model: DDT.SortFilterProxyModel {
                sourceModel: root.model
                filterRowCallback: (sourceRow, sourceParent) => {
                    let index = sourceModel.index(sourceRow, 0, sourceParent)
                    return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) !== "stashed" &&
                           sourceModel.data(index, DDT.TraySortOrderModel.VisibilityRole) === true
                }
            }
            delegate: TrayItemDelegateChooser {
                id: delegateRoot
                isHorizontal: root.isHorizontal
                collapsed: root.collapsed
                itemPadding: root.itemPadding
                surfaceAcceptor: root.surfaceAcceptor
                disableInputEvents: root.isDropping

                property int visualIndex: DelegateModel.itemsIndex
            }
        }

        add: Transition {
            enabled: animationEnable
            NumberAnimation {
                properties: "scale,opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        remove: Transition {
            enabled: animationEnable
            NumberAnimation {
                properties: "scale,opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
        displaced: Transition {
            enabled: animationEnable
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutQuad
            }
        }
        move: displaced
    }

    Component.onCompleted: {
        DDT.TrayItemPositionManager.orientation = Qt.binding(function() {
            return root.isHorizontal ? Qt.Horizontal : Qt.Vertical
        });
        DDT.TrayItemPositionManager.visualItemCount = Qt.binding(function() {
            return root.model.rowCount
        });
        DDT.TrayItemPositionManager.dockHeight = Qt.binding(function() {
            return root.trayHeight
        });

        animationEnableTimer.start()
    }
}
