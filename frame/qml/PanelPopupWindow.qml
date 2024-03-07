// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import org.deepin.dtk 1.0 as D

Window {
    property real xOffset: 0
    property real yOffset: 0

    flags: Qt.Popup
    D.DWindow.enabled: true
    D.DWindow.windowRadius: 18
    x: transientParent.x + xOffset
    y: transientParent.y + yOffset
}
