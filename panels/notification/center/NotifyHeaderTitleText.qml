// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import Qt5Compat.GraphicalEffects
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

DropShadowText {
    property font tFont: DTK.fontManager.t4
    font {
        pixelSize: tFont.pixelSize
        family: tFont.family
        bold: true
    }
}
