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

    x: isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    y: !isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    z: 5

    width: 16
    height: 16
    icon.name: isHorizontal ? (collapsed ? "expand-left" : "expand-right") : (collapsed ? "expand-up" : "expand-down")
    icon.width: width
    icon.height: height
    display: D.IconLabel.IconOnly

    onClicked: {
        DDT.TraySortOrderModel.collapsed = !DDT.TraySortOrderModel.collapsed
    }

    Behavior on x {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
    Behavior on y {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
}
