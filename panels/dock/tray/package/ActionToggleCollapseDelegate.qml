// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock.tray 1.0 as DDT

D.ToolButton {
    property bool isHorizontal: false
    property bool collapsed: DDT.TraySortOrderModel.collapsed

    z: 5

    icon.name: isHorizontal ? (collapsed ? "expand-left" : "expand-right") : (collapsed ? "expand-up" : "expand-down")
    icon.width: 16
    icon.height: 16
    display: D.IconLabel.IconOnly

    topPadding: itemPadding
    bottomPadding: itemPadding
    leftPadding: itemPadding
    rightPadding: itemPadding

    onClicked: {
        DDT.TraySortOrderModel.collapsed = !DDT.TraySortOrderModel.collapsed
    }
}
