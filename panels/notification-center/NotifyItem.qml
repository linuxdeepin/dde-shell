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
    property bool strongInteractive: false
    property string contentIcon: "deepin-editor"

    signal remove()
    signal setting(var pos)
    signal actionInvoked(var actionId)
}
