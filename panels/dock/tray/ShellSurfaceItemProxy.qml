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
    
    implicitWidth: shellSurface ? shellSurface.width : 10
    implicitHeight: shellSurface ? shellSurface.height : 10

    function takeFocus() {
        impl.takeFocus()
    }

    function fixPosition() {
        positionFixer.fix()
    }

    PositionFixer {
        id: positionFixer
        item: impl
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
            cursorShape: root.cursorShape
        }
        TapHandler {
            id: tapHandler
        }

        onVisibleChanged: function () {
            if (visible) {
                positionFixer.fix()
            }

            if (autoClose && !visible) {
                Qt.callLater(closeShellSurface)
            }
        }
        function closeShellSurface()
        {
            if (surface && shellSurface) {
                DockCompositor.closeShellSurface(shellSurface)
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

        function onCursorShapeRequested(cursorShape)
        {
            console.log("onCursorShapeRequested:", cursorShape)
            // Qt::CursorShape range is 0-21, plus 24 (BitmapCursor) and 25 (CustomCursor).
            // We set a default if the value is out of logical bounds.
            if (cursorShape < 0 || cursorShape > 25) {
                root.cursorShape = Qt.ArrowCursor
            } else {
                root.cursorShape = cursorShape
            }
        }
    }
}
