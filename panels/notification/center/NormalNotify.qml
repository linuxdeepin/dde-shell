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
    implicitWidth: impl.implicitWidth
    implicitHeight: impl.implicitHeight

    signal gotoNextItem()
    signal gotoPrevItem()

    function focusFirstButton() {
        return notifyContent.focusFirstButton()
    }

    Control {
        id: impl
        anchors.fill: parent

        contentItem: NotifyItemContent {
            id: notifyContent
            width: parent.width
            appName: root.appName
            iconName: root.iconName
            content: root.content
            title: root.title
            date: root.date
            actions: root.actions
            defaultAction: root.defaultAction
            parentHovered: impl.hovered || root.activeFocus
            strongInteractive: root.strongInteractive
            contentIcon: root.contentIcon
            contentRowCount: root.contentRowCount
            indexInGroup: root.indexInGroup

            onRemove: function () {
                root.remove()
            }
            onDismiss: function () {
                root.dismiss()
            }
            onActionInvoked: function (actionId) {
                root.actionInvoked(actionId)
            }
            onGotoNextItem: root.gotoNextItem()
            onGotoPrevItem: root.gotoPrevItem()
        }
    }
}
