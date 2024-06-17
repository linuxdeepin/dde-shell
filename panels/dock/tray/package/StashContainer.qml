// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1 as LP
import org.deepin.ds.dock.tray 1.0 as DDT

Item {
    id: root

    property var model: ListModel {
        ListElement {
            delegateType: "dummy"
            surfaceId: "folder-trash"
            visualIndex: 0
        }
        ListElement {
            delegateType: "dummy"
            visualIndex: 1
        }
    }

    property string color: "red"

    readonly property int itemSize: 16
    readonly property int itemSpacing: 10

    readonly property int columnCount: Math.ceil(Math.sqrt(model.count))
    readonly property int rowCount: Math.round(Math.sqrt(model.count))

    implicitWidth: width
    width: columnCount * (itemSize + itemSpacing) - itemSpacing
    implicitHeight: height
    height: rowCount * (itemSize + itemSpacing) - itemSpacing

    Behavior on width {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    // Delegates
    StashedItemDelegateChooser {
        columnCount: root.columnCount
        rowCount: root.rowCount
        id: stashedItemDelegateChooser
    }

    // debug
    Rectangle {
        color: root.color
        anchors.fill: parent
    }

    // Tray items
    Repeater {
        anchors.fill: parent
        model: root.model
        delegate: stashedItemDelegateChooser
    }
}
