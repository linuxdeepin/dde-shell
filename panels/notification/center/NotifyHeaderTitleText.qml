// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import Qt5Compat.GraphicalEffects
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

Text {
    property font tFont: DTK.fontManager.t4
    font {
        pixelSize: tFont.pixelSize
        family: tFont.family
        bold: true
    }
    color: Qt.rgba(1, 1, 1, 1)
    layer.enabled: true
    layer.effect: DropShadow {
        color: Qt.rgba(0, 0, 0, 0.6)
        radius: 4
        samples: 9
        verticalOffset: 1
    }
}
