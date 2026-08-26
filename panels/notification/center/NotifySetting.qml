// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

NotifySettingMenu {
    Accessible.role: Accessible.Pane
    Accessible.id: "NotifySettingMenu"
    id: root

    required property NotifyModel notifyModel
    property bool pinned
    property string appName

    MenuItem {
        Accessible.id: "Unpin"
        text: pinned ? qsTr("Unpin") : qsTr("Pin")
        onClicked: {
            let state = !root.pinned
            console.log("Pin changed", state)
            notifyModel.pinApplication(appName, state)
        }
    }
    MenuItem {
        Accessible.id: "NotificationSetting"
        text: qsTr("Notification Setting")
        onClicked: {
            console.log("Notify setting")
            NotifyAccessor.openNotificationSetting()
        }
    }
}
