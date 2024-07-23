// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

PanelPopupWindow {
    id: root

    flags: Qt.Tool | Qt.WindowStaysOnTopHint
    D.DWindow.windowRadius: 4
    D.DWindow.shadowRadius: 8
}
