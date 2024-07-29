// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock.tray 1.0 as DDT
import org.deepin.ds.dock 1.0

AppletItemButton {
    property bool isHorizontal: false
    property bool collapsed: DDT.TraySortOrderModel.collapsed

    z: 5

    icon.name: isHorizontal ? (collapsed ? "expand-left" : "expand-right") : (collapsed ? "expand-up" : "expand-down")

    padding: itemPadding

    onClicked: {
        DDT.TraySortOrderModel.collapsed = !DDT.TraySortOrderModel.collapsed
    }
}
