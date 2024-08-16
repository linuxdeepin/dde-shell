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

    required property DDT.SortFilterProxyModel model: DDT.SortFilterProxyModel {
        sourceModel: DDT.TraySortOrderModel
        filterRowCallback: (sourceRow, sourceParent) => {
            let index = sourceModel.index(sourceRow, 0, sourceParent)
            return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) === "stashed" &&
                    sourceModel.data(index, DDT.TraySortOrderModel.VisibilityRole) === true
        }
    }

    property string color: "red"

    readonly property int itemSize: 16
    readonly property int itemSpacing: 10
    readonly property int itemPadding: 8

    readonly property int columnCount: Math.ceil(Math.sqrt(model.count))
    readonly property int rowCount: Math.round(Math.sqrt(model.count))
    property bool dropHover: false
    property bool stashItemDragging: false

    function isStashPopup(surfaceId)
    {
        for (let i = model.rowCount() - 1; i >= 0; i--) {
            let index = model.index(i, 0)
            let data = model.data(index, DDT.TraySortOrderModel.surfaceId)
            if (data === surfaceId)
                return true
        }
        return false
    }

    implicitWidth: width
    width: columnCount * (itemSize + itemPadding * 2 + itemSpacing) - itemSpacing
    implicitHeight: height
    height: rowCount * (itemSize + itemPadding * 2 + itemSpacing) - itemSpacing

    Behavior on width {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    // Delegates
    // tooltip and menu
    DDT.SurfacePopup {
        id: stashSurfacePopup
        objectName: "stash"
        surfaceAcceptor: isStashPopup
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
            let sectionType = dragEvent.getDataAsString("text/x-dde-shell-tray-dnd-sectionType")
            let surfaceId = dragEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId")
            dropHover = true
            stashItemDragging = sectionType === "stashed"
            if (!surfaceId.startsWith("application-tray")) {
                dragEvent.accepted = false
            }
        }
        onExited: function (dragEvent) {
            dropHover = false
        }
        onPositionChanged: function (dragEvent) {
            let surfaceId = dragEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId")
            console.log("dragging", surfaceId)
        }
        onDropped: function (dropEvent) {
            let surfaceId = dropEvent.getDataAsString("text/x-dde-shell-tray-dnd-surfaceId")
            DDT.TraySortOrderModel.move(surfaceId, "internal/action-stash-placeholder", true);
        }
    }

    Flow {
        id: stashedList
        anchors.fill: parent
        anchors.margins: 0
        spacing: root.itemSpacing

        add: Transition {
            NumberAnimation {
                properties: "scale,opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutQuad
            }
        }

        Repeater {
            model: DelegateModel {
                id: visualModel
                model: root.model
                delegate: StashedItemDelegateChooser {
                    columnCount: root.columnCount
                    rowCount: root.rowCount
                    itemPadding: root.itemPadding
                    id: stashedItemDelegateChooser
                    stashedSurfacePopup: stashSurfacePopup
                }
            }
        }
    }
}
