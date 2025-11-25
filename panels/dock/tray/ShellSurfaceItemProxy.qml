// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
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

    implicitWidth: shellSurface ? shellSurface.width : 10
    implicitHeight: shellSurface ? shellSurface.height : 10

    function takeFocus() {
        impl.takeFocus()
    }

    ShellSurfaceItem {
        id: impl
        width: parent.width
        height: parent.height
        shellSurface: root.shellSurface
        inputEventsEnabled: root.inputEventsEnabled
        // we need to set smooth to false, otherwise the image
        // will be blurred if the scale is 1.25.
        // If the surface width is 150, the buffer width will be 150 * 1.25 = 188
        // But the ShellSurfaceItem pixel width on screen is 150 * 1.25 = 187.5
        // So Qt will use the 188 to scale to 187.5, which will be blurred.
        // But if we set smooth to false, the Qt doesn't linear interpolation.
        // TODO: If the buffer size greater than the ShellSurfaceItem pixel
        // size, we also need enable smooth.
        smooth: false

        HoverHandler {
            id: hoverHandler
        }
        TapHandler {
            id: tapHandler
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
}
