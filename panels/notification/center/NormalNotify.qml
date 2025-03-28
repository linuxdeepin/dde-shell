// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root

    property var removedCallback

    states: [
        State {
            name: "removing"
            PropertyChanges { target: root; x: root.width; opacity: 0}
        }
    ]

    transitions: Transition {
        to: "removing"
        ParallelAnimation {
            NumberAnimation { properties: "x"; duration: 300; easing.type: Easing.Linear }
            NumberAnimation { properties: "opacity"; duration: 300; easing.type: Easing.Linear }
        }
        onRunningChanged: {
            if (!running && root.removedCallback) {
                root.removedCallback()
                root.removedCallback = undefined
            }
        }
    }

    contentItem: NotifyItemContent {
        width: parent.width
        appName: root.appName
        iconName: root.iconName
        content: root.content
        title: root.title
        date: root.date
        actions: root.actions
        defaultAction: root.defaultAction
        closeVisible: root.hovered || root.activeFocus
        strongInteractive: root.strongInteractive
        contentIcon: root.contentIcon
        contentRowCount: root.contentRowCount

        onRemove: function () {
            root.removedCallback = function () {
                root.remove()
            }
            root.state = "removing"
        }
        onDismiss: function () {
            root.removedCallback = function () {
                root.dismiss()
            }
            root.state = "removing"
        }
        onActionInvoked: function (actionId) {
            root.actionInvoked(actionId)
        }
    }
}
