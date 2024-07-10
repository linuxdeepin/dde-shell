// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

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
        if (surface && shellSurface) {
            shellSurface.close()
        }
    }
}
