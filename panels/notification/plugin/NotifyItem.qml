// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification

FocusScope {
    id: root

    enum CloseReason {
        Expired = 1,
        Dismissed = 2,
        Closed = 3,
        Unknown = 4
    }

    property string appName: "deepin-editor"
    property string iconName: "deepin-editor"
    property string content: "content"
    property string title: "title"
    property string date: "date"
    property var actions: [
        {text: "open", id: "open"},
        {text: "close", id: "close"},
        {text: "exec", id: "exec"}
    ]
    property string defaultAction
    property bool strongInteractive: false
    property string contentIcon: "deepin-editor"
    property int contentRowCount: 6
    property int indexInGroup: -1

    signal remove()
    signal dismiss()
    signal setting(var pos)
    signal actionInvoked(var actionId)
    signal linkActivated(var link)

    onLinkActivated: function (link) {
        console.log("Link actived", link)
        ApplicationHelper.openUrl(link)
    }
}
