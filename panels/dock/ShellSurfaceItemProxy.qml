// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import org.deepin.ds.dock 1.0
import org.deepin.ds 1.0

Item {
    id: root
    property var shellSurface: null
    signal surfaceDestroyed()
    property bool autoClose: false
    property bool inputEventsEnabled: true
    property bool hovered: hoverHandler.hovered
    property bool pressed: tapHandler.pressed
    property int cursorShape: Qt.ArrowCursor
    property alias shellSurfaceItem: impl
    readonly property bool hasSurfaceContent: impl.surface ? impl.surface.hasContent : false

    implicitWidth: shellSurface ? shellSurface.width : 16
    implicitHeight: shellSurface ? shellSurface.height : 16

    Accessible.name: shellSurface ? (shellSurface.displayName || "") : ""

    function takeFocus() {
        impl.takeFocus()
    }

    function fixPosition() {
        fixPositionTimer.start()
    }

    ShellSurfaceItem {
        id: impl
        width: parent.width
        Accessible.id: "ShellSurfaceItemProxy"
        height: parent.height
        shellSurface: root.shellSurface
        inputEventsEnabled: root.inputEventsEnabled
        // When the devicePixelRatio is non-integer (e.g. 1.25), the buffer
        // size (round(w * dpr)) may not match the display pixel size (w * dpr).
        // In that case, smooth must be true to avoid nearest-neighbor distortion.
        // When they match (integer DPR), keep smooth false to avoid blurring.
        smooth: {
            if (!impl.surface)
                return false
            var dpr = Panel.devicePixelRatio
            return Math.round(impl.width * dpr) !== impl.width * dpr
                || Math.round(impl.height * dpr) !== impl.height * dpr
        }

        HoverHandler {
            id: hoverHandler
            cursorShape: root.cursorShape
        }
        TapHandler {
            id: tapHandler
            onLongPressed: {
                if (tapHandler.point.device.type === PointerDevice.TouchScreen) {
                    DockCompositor.sendRightClickForSurface()
                }
            }
        }

        onVisibleChanged: function () {
            if (visible) {
                fixPositionTimer.start()
            }

            if (autoClose && !visible) {
                // surface is valid but client's shellSurface maybe invalid.
                Qt.callLater(closeShellSurface)
            }
        }
        function closeShellSurface()
        {
            if (surface && shellSurface) {
                DockCompositor.closeShellSurface(shellSurface)
            }
        }

        function mapToScene(x, y) {
            const point = Qt.point(x, y)
            // Must use parent.mapFoo, because the impl's position is relative to the parent Item
            const mappedPoint = parent.mapToItem(Window.window.contentItem, point)
            return mappedPoint
        }

        function mapFromScene(x, y) {
            const point = Qt.point(x, y)
            // Must use parent.mapFoo, because the impl's position is relative to the parent Item
            const mappedPoint = parent.mapFromItem(Window.window.contentItem, point)
            return mappedPoint
        }

        function fixPosition() {
            // See QTBUG: https://bugreports.qt.io/browse/QTBUG-135833
            // TODO: should get the devicePixelRatio from the Window
            x = mapFromScene(Math.ceil(mapToScene(0, 0).x * Panel.devicePixelRatio) / Panel.devicePixelRatio, 0).x
            y = mapFromScene(0, Math.ceil(mapToScene(0, 0).y * Panel.devicePixelRatio) / Panel.devicePixelRatio).y
        }

        Timer {
            id: fixPositionTimer
            interval: 100
            repeat: false
            running: false
            onTriggered: {
                impl.fixPosition()
            }
        }
    }
    Component.onCompleted: function () {
        impl.surfaceDestroyed.connect(root.surfaceDestroyed)
        updateCursorShapeConnection()
    }

    Connections {
        target: shellSurface
        // TODO it's maybe a bug for qt, we force shellSurface's value to update
        function onAboutToDestroy()
        {
            Qt.callLater(function() {
                impl.shellSurface = null
                impl.shellSurface = Qt.binding(function () {
                    return root.shellSurface
                })
            })
        }
    }

    property var cursorShapeConnection: null

    onShellSurfaceChanged: updateCursorShapeConnection()

    function updateCursorShapeConnection() {
        if (cursorShapeConnection) {
            cursorShapeConnection.disconnect()
            cursorShapeConnection = null
        }
        // cursorShapeRequested only exists on PluginPopup, not on PluginSurface,
        // so connect dynamically to avoid a QML warning for surfaces lacking it.
        if (shellSurface && shellSurface.cursorShapeRequested) {
            cursorShapeConnection = shellSurface.cursorShapeRequested.connect(onCursorShapeRequested)
        }
    }

    function onCursorShapeRequested(cursorShape) {
        // Qt::CursorShape range is 0-21, plus 24 (BitmapCursor) and 25 (CustomCursor).
        // We set a default if the value is out of logical bounds.
        if (cursorShape < 0 || cursorShape > 25) {
            root.cursorShape = Qt.ArrowCursor
        } else {
            root.cursorShape = cursorShape
        }
    }
}
