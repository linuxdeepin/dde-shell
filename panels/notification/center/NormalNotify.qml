// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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

    function focusLastButton() {
        return notifyContent.focusLastButton()
    }

    Control {
        id: impl
        anchors.fill: parent
        focus: true

        Keys.onTabPressed: function(event) {
            if (!notifyContent.focusFirstButton()) {
                root.gotoNextItem()
            }
            event.accepted = true
        }

        Keys.onBacktabPressed: function(event) {
            if (!notifyContent.focusLastButton()) {
                root.gotoPrevItem()
            }
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
            parentHovered: impl.hovered || root.activeFocus
            strongInteractive: root.strongInteractive
            contentIcon: root.contentIcon
            contentRowCount: root.contentRowCount
            indexInGroup: root.indexInGroup
            background: NotifyItemBackground {
                backgroundColor: Palette {
                    normal {
                        common: ("transparent")
                        crystal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.7)
                    }
                    normalDark {
                        crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 0.7)
                    }
                }
            }

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
