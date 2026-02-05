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

    property int count: 1
    readonly property int overlapItemRadius: 12
    property bool enableDismissed: true
    property bool shouldShowClose: false  // True when item gets focus from keyboard navigation
    property var removedCallback
    property alias notifyContent: notifyContent

    signal expand()
    signal gotoNextItem()
    signal gotoPrevItem()

    property var clearButton: null  // Reference to the externally defined clear button

    function focusFirstButton() {
        // Focus clear button first, then action buttons
        if (clearButton && clearButton.enabled && clearButton.visible) {
            clearButton.forceActiveFocus()
            return true
        }
        // Try action buttons (skip clear button to avoid loop)
        return notifyContent.focusFirstActionOnly()
    }

    function focusLastButton() {
        // Focus last action button, then clear button
        if (notifyContent.focusLastButton()) {
            return true
        }
        // Try clear button
        if (clearButton && clearButton.enabled && clearButton.visible) {
            clearButton.forceActiveFocus()
            return true
        }
        return false
    }

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

    Control {
        id: impl
        anchors.fill: parent
        focus: true

        Keys.onTabPressed: function(event) {
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

        contentItem: Item {
            width: parent.width
            implicitHeight: notifyContent.height + indicator.height
            NotifyItemContent {
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
                enableDismissed: root.enableDismissed
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

            OverlapIndicator {
                id: indicator
                enableAnimation: root.ListView.view.panelShown
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    leftMargin: overlapItemRadius
                    right: parent.right
                    rightMargin: overlapItemRadius
                }
                z: -1
                count: root.count
            }
        }

        // expand
        TapHandler {
            enabled: !root.enableDismissed
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.forceActiveFocus()
                root.expand()
            }
        }
        Keys.onEnterPressed: root.expand()
        Keys.onReturnPressed: root.expand()
    }
}
