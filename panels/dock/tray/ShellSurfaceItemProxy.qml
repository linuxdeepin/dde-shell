// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import org.deepin.ds.dock 1.0

ShellSurfaceItem {
    property bool autoClose: false
    onVisibleChanged: function () {
        if (autoClose && !visible) {
            // surface is valid but client's shellSurface maybe invalid.
            Qt.callLater(closeShellSurface)
        }
    }
    function closeShellSurface()
    {
        if (surface && shellSurface && output.window && output.window.visible) {
            DockCompositor.closeShellSurface(shellSurface)
        }
    }
}
