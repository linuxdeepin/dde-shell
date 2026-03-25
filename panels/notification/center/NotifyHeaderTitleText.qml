// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import Qt5Compat.GraphicalEffects
import org.deepin.dtk 1.0 
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

DropShadowText {
    property font tFont: DTK.fontManager.t4
    color: DTK.themeType === ApplicationHelper.DarkType ?
        Qt.rgba(255, 255, 255, 0.7) :
        Qt.rgba(0, 0, 0, 0.75)
    font {
        pixelSize: tFont.pixelSize
        family: tFont.family
        bold: true
    }
}
