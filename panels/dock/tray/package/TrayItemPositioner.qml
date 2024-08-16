// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.ds.dock.tray 1.0 as DDT

Control {
    id: root
    property bool itemVisible: true
    property size visualSize: Qt.size(0, 0)

    property bool isDragging: DDT.TraySortOrderModel.actionsAlwaysVisible

    DDT.TrayItemPositionRegister.visualIndex: DelegateModel.visualIndex
    DDT.TrayItemPositionRegister.visualSize: (model.sectionType !== "stashed") ? Qt.size(width, height) : Qt.size(0, 0)
    DDT.TrayItemPositionRegister.surfaceId: model.surfaceId
    DDT.TrayItemPositionRegister.sectionType: model.sectionType

    width: visualSize.width !== 0 ? visualSize.width : DDT.TrayItemPositionManager.itemVisualSize.width
    height: visualSize.height !== 0 ? visualSize.height : DDT.TrayItemPositionManager.itemVisualSize.height

    anchors.verticalCenter: isHorizontal ? parent.verticalCenter : undefined
    anchors.horizontalCenter: isHorizontal ? undefined : parent.horizontalCenter
}
