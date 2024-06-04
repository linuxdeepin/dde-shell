// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.ds.dock.tray.quickpanel 1.0

Item {
    id: root

    required property Item dragItem

    Component.onCompleted: {
        dragItem.Drag.keys = [
            "text/x-dde-launcher-dnd-desktopId"
        ]
        dragItem.Drag.mimeData = {
            "text/x-dde-launcher-dnd-desktopId": "dde-calendar.desktop"
        }
        dragItem.Drag.dragType = Drag.Automatic
        dragItem.Drag.supportedActions = Qt.CopyAction
        dragItem.Drag.supportedActions = Qt.CopyAction
        dragItem.DQuickDrag.hotSpotScale = Qt.binding(function () {
            return Qt.size(0.5, 0.5)
        })
        dragItem.DQuickDrag.active = Qt.binding(function () {
            return dragItem.Drag.active
        })
        dragItem.DQuickDrag.overlay = Qt.binding(function () {
            return overlayWindow
        })
    }

    property string fallbackDragImage: "computer-symbolic"
    property string draggingImage
    property size fallbackIconSize: Qt.size(16, 16)

    property Component overlayWindow: QuickDragWindow {
        property size dragItemSize
        property string iconName
        property point startDragPoint
        property point currentDragPoint
        property int endDragPointArea
        property bool isFallbackIcon: {
            return height <= root.fallbackIconSize.height
        }

        startDragPoint: dragItem.DQuickDrag.startDragPoint
        currentDragPoint: dragItem.DQuickDrag.currentDragPoint
        dragItemSize: Qt.size(dragItem.width, dragItem.height)
        endDragPointArea: {
            return Panel.rootObject.y - root.fallbackIconSize.height * dragItem.DQuickDrag.hotSpotScale.height
        }

        // Height and position follow linear transformation, y = k * h + b.
        // and (y, h) exist two rules, (start.y, drag.h) and (end.y, 16)
        function getHeight() {
            if (currentDragPoint.y < startDragPoint.y)
                return dragItemSize.height
            if (currentDragPoint.y > endDragPointArea)
                return root.fallbackIconSize.height

            var k = (startDragPoint.y - endDragPointArea) * 1.0 / (dragItemSize.height - root.fallbackIconSize.height)
            var b = startDragPoint.y - k * dragItemSize.height * 1.0
            var h = (currentDragPoint.y * 1.0 - b) / k
            return h
        }

        iconName: !isFallbackIcon ? root.draggingImage : root.fallbackDragImage
        height: getHeight()
        width: (dragItemSize.width * 1.0 / dragItemSize.height) * height

        DciIcon {
            id: iconView
            anchors.fill: parent
            sourceSize: Qt.size(width, height)
            asynchronous: false
            name: iconName
        }
    }

    DragHandler {
        id: dragHandler
        // dragThreshold: 1
        onActiveChanged: {
            if (active) {
                dragItem.grabToImage(function(result) {
                    console.log("grab to image", result.url)

                    var local = "/tmp/" + result.url + ".png"
                    local = local.replace(/#/g, '-')
                    result.saveToFile(local)

                    draggingImage = "file://" + local
                })
            }
            dragItem.Drag.active = active
        }
    }
}
