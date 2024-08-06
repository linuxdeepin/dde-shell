// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

PanelPopupWindow {
    id: root

    flags: (Qt.platform.pluginName === "xcb" ? Qt.Tool | Qt.X11BypassWindowManagerHint : Qt.ToolTip) | Qt.WindowStaysOnTopHint
    property var mainWindow
}
