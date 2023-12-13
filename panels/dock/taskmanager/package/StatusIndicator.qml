// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    required property AppItemPalette palette
    width: 30
    height: 30
    radius: 6
    color: palette.rectIndicator
    border.width: 1
    border.color: palette.rectIndicatorBorder
}
