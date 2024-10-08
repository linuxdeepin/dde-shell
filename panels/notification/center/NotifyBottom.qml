// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

Control {
    id: root

    required property NotifyModel notifyModel

    contentItem: Button {
        objectName: "expandAllNotifycation"
        text: qsTr("%1 more notifications").arg(notifyModel.remainCount)
        onClicked: {
            console.log("All notify")
            notifyModel.expandAllApp()
        }
    }

    background: BoundingRectangle {}
}
