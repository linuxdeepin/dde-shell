// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.ds.dock 1.0

Item {

    required property Item dragItem

    Component.onCompleted: {
        dragItem.Drag.keys = [
            "text/x-dde-dock-dnd-appid"
        ]
        dragItem.Drag.mimeData = {
            "text/plain": "Copied text"
        }
        dragItem.Drag.dragType = Drag.Automatic
        dragItem.Drag.supportedActions = Qt.CopyAction
        dragItem.Drag.supportedActions = Qt.CopyAction
        dragItem.Drag.hotSpot.x = Qt.binding(function () {
            return dragItem.width / 2
        })
        dragItem.Drag.hotSpot.y = Qt.binding(function () {
            return dragItem.height / 2
        })
        dragItem.Drag.imageSource = Qt.binding(function () {
            return draggingUrl
        })
    }

    property url draggedUrl
    property url draggingUrl: draggedUrl

    DragHandler {
        id: dragHandler
        // yAxis.enabled: true
        // yAxis.maximum: 100
        // dragThreshold: 1
        onActiveChanged: {
            if (active) {
                dragItem.grabToImage(function(result) {
                    console.log("*********", result.url, dragHandler.dragThreshold)
                    // quickpanelItem.Drag.imageSource = result.url;
                    draggedUrl = result.url
                    // dragPlaceholder.source = result.url
                   // dragIcon.source = result.url;
                })
            }
            console.log("**************", active, dragItem.Drag.imageSource)
            dragItem.Drag.active = active
        }
    }
    Timer {
        repeat: true
        running: true
        interval: 1000
        property int count: 1
        onTriggered: {
            count ++
            // if (!dragHandler.active)
            //     return
            // draggingUrl = count % 2 ? draggedUrl : "file:///usr/share/icons/bloom/places/32/deepin-launcher.svg"
            // console.log("****", quickpanelItem.Drag.imageSource)
            // dragPlaceholder.source = dragIcon.url()
        }
    }
}
