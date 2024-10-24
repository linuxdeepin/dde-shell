// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.ds.dock.tray 1.0
import org.deepin.ds.dock.tray.quickpanel 1.0

Item {
    id: root

    required property Item dragItem
    required property string dragTextData
    required property var fallbackDragImage
    property bool enabledDrag

    Component.onCompleted: {
        dragItem.Drag.mimeData = Qt.binding(function () {
            return {
                "text/x-dde-shell-tray-dnd-surfaceId": root.dragTextData
            }
        })
        dragItem.Drag.dragType = Drag.Automatic
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

    property string draggingImage
    property size fallbackIconSize: Qt.size(16, 16)

    property Component overlayWindow: QuickDragWindow {
        id: overlayWindow
        property size dragItemSize
        property point startDragPoint
        property point currentDragPoint
        property int endDragPointArea
        // This no longer changes window size (due to visual artifacts), but rather controls component image size.
        property real overlayWindowHeight: getHeight()
        property bool isFallbackIcon: {
            return overlayWindowHeight <= root.fallbackIconSize.height
        }

        startDragPoint: dragItem.DQuickDrag.startDragPoint
        currentDragPoint: dragItem.DQuickDrag.currentDragPoint
        dragItemSize: Qt.size(dragItem.width, dragItem.height)
        endDragPointArea: {
            return Panel.rootObject.y - (root.fallbackIconSize.height + dragItem.height) * dragItem.DQuickDrag.hotSpotScale.height
        }

        // Height and position follow linear transformation, y = k * h + b.
        // and (y, h) exist two rules, (start.y, drag.h) and (end.y, 16)
        function getHeight() {
            console.log(currentDragPoint)
            if (currentDragPoint.y < startDragPoint.y)
                return dragItemSize.height
            if (currentDragPoint.y > endDragPointArea)
                return root.fallbackIconSize.height

            var k = (startDragPoint.y - endDragPointArea) * 1.0 / (dragItemSize.height - root.fallbackIconSize.height)
            var b = startDragPoint.y - k * dragItemSize.height * 1.0
            var h = (currentDragPoint.y * 1.0 - b) / k
            return h
        }

        // FIXME: Since we attempted fixing the drag jitter of quick controls (caused by the constantly changing window
        // size & position, and the composition delays following that) by using a fixed window size and change only
        // the drag image size, the old blur-behind implementation will no longer work (blur area will be larger than
        // the drag image). Using StyledBehindWindowBlur to fill the Image component will result in inconsistent corner
        // radius, so that part is also not enabled in the code.
        // TODO: turning this on results in the input transparency property of the drag window not being respected,
        // causing DnD events being delivered to not the tray, but the drag window itself. Using StyledBehindWindowBlur
        // to fill the Image component also causes this bug, so blur-behind is currently not enabled.
        // DWindow.enabled: true
        // DWindow.enableBlurWindow: !isFallbackIcon
        // DWindow.shadowRadius: 0
        // DWindow.borderWidth: 0
        ColorSelector.family: Palette.CrystalColor

        height: dragItemSize.height
        width: dragItemSize.width


        Loader {
            active: !isFallbackIcon
            anchors.centerIn: parent
            sourceComponent: Image {
                source: root.draggingImage
                height: overlayWindow.overlayWindowHeight
                width: (dragItemSize.width * 1.0 / dragItemSize.height) * height

                // StyledBehindWindowBlur {
                //     control: parent
                //     anchors.fill: parent
                // }
            }
        }

        Loader {
            active: isFallbackIcon && root.dragItem.DQuickDrag.isDragging
            anchors.centerIn: parent
            sourceComponent: ShellSurfaceItemProxy {
                shellSurface: root.fallbackDragImage
                width: root.fallbackIconSize.width
                height: root.fallbackIconSize.height
            }
        }
    }

    DragHandler {
        id: dragHandler
        enabled: enabledDrag
        dragThreshold: 5
        onActiveChanged: {
            if (active) {
                dragItem.grabToImage(function(result) {
                    console.log("grab to image", result.url)

                    draggingImage = result.url
                })
            }
            dragItem.Drag.active = active
        }
    }
}
