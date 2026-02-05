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
    property bool shouldShowClose: false  // True when item gets focus from keyboard navigation

    signal gotoNextItem()
    signal gotoPrevItem()

    function focusFirstButton() {
        return notifyContent.focusFirstButton()
    }

    function focusLastButton() {
        return notifyContent.focusLastButton()
    }

    Control {
        id: impl
        anchors.fill: parent
        focus: true

        Keys.onTabPressed: function(event) {
            // Mark that this item got focus from Tab navigation
            root.shouldShowClose = true
            if (notifyContent.focusFirstButton()) {
                event.accepted = true
            } else {
                root.gotoNextItem()
                event.accepted = true
            }
        }

        Keys.onBacktabPressed: function(event) {
            root.shouldShowClose = true
            root.gotoPrevItem()
            event.accepted = true
        }

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
            // Show close button when: mouse hovers, or item has focus from keyboard navigation
            parentHovered: impl.hovered || (root.activeFocus && root.shouldShowClose)
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
