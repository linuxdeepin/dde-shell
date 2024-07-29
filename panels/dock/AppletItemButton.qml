// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.ds.dock 1.0
import org.deepin.dtk

IconButton {
    id: control
    property bool isActive
    property real radius: 4

    padding: 4
    topPadding: undefined
    bottomPadding: undefined
    leftPadding: undefined
    rightPadding: undefined

    textColor: DockPalette.iconTextPalette
    display: IconLabel.IconOnly

    icon.width: 16
    icon.height: 16

    background: AppletItemBackground {
        radius: control.radius
        isActive: control.isActive
    }
}
