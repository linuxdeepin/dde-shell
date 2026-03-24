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

    property int count: 1
    readonly property int overlapItemRadius: 12
    property bool enableDismissed: true
    property var removedCallback
    property alias notifyContent: notifyContent

    property bool focusedByNavigation: false
    onActiveFocusChanged: if (!activeFocus) focusedByNavigation = false

    signal expand()
    signal gotoNextItem()
    signal gotoPrevItem()

    property var clearButton: notifyContent.clearButtonItem

    function resetFocus() {
        impl.forceActiveFocus()
    }

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
                parentHovered: impl.hovered || (root.activeFocus && root.focusedByNavigation)
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

                background: NotifyItemBackground {
                    backgroundColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.7)
                        }
                        normalDark {
                            crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 0.7)
                        }
                    }
                }
            }

            OverlapIndicator {
                id: indicator
                enableAnimation: root.ListView.view.panelShown
                clipItems: true
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    leftMargin: overlapItemRadius
                    right: parent.right
                    rightMargin: overlapItemRadius
                }
                z: -1
                count: root.count
                background: NotifyItemBackground {
                    opacity: realIndex === 0 ? 0.6 : 0.4
                    backgroundColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.7)
                        }
                        normalDark {
                            crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 0.7)
                        }
                    }
                    borderColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(0, 0, 0, 0.1)
                        }
                        normalDark {
                            crystal: Qt.rgba(0, 0, 0, 0.6)
                        }
                    }
                    insideBorderColor: null
                    outsideBorderColor: null
                    dropShadowColor: null
                }
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

        TapHandler {
            acceptedButtons: Qt.RightButton
            onPressedChanged: function () {
                if (pressed) {
                    let pos = point.position
                    root.setting(pos)
                }
            }
        }

        Keys.onEnterPressed: root.expand()
        Keys.onReturnPressed: root.expand()
    }
}
