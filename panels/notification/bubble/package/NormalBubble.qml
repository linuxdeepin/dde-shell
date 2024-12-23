// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.notification 1.0
import org.deepin.dtk 1.0 as D

NotifyItemContent {
    id: control
    property var bubble

    width: 360
    appName: bubble.appName
    iconName: bubble.iconName
    date: bubble.timeTip
    actions: bubble.actions
    defaultAction: bubble.defaultAction
    title: bubble.summary
    content: bubble.body
    strongInteractive: bubble.urgency === 2
    contentIcon: bubble.bodyImagePath
    contentRowCount: bubble.contentRowCount
    onRemove: function () {
        console.log("remove notify", bubble.appName)
        Applet.close(bubble.index, NotifyItem.Closed)
    }
    onDismiss: function () {
        console.log("dismiss notify", bubble.appName)
        Applet.close(bubble.index, NotifyItem.Dismissed)
    }
    onActionInvoked: function (actionId) {
        console.log("action notify", bubble.appName, actionId)
        Applet.invokeAction(bubble.index, actionId)
    }
}
