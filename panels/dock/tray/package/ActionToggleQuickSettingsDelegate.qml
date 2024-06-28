// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Button {
    property bool isHorizontal: false
    icon.name: "dock-control-panel"
    icon.width: 16
    icon.height: 16
    width: isHorizontal ? 26 : 16
    height: isHorizontal ? 16 : 26
    onClicked: {
        // toggle quick settings
    }
}
