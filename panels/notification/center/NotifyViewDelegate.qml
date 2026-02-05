// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

DelegateChooser {
    id: root
    required property NotifyModel notifyModel
    required property Item view

    signal setting(var pos, var params)

    role: "type"

    // Shared navigation helper: focus next item or cycle to header
    function navigateToNextItem(currentIndex) {
        if (currentIndex < view.count - 1) {
            view.currentIndex = currentIndex + 1
            view.positionViewAtIndex(currentIndex + 1, ListView.Contain)
            Qt.callLater(function() {
                let nextItem = view.itemAtIndex(currentIndex + 1)
                if (nextItem && nextItem.enabled) {
                    // Focus on the item itself first, not directly to buttons
                    nextItem.forceActiveFocus()
                }
            })
        } else {
            view.gotoHeaderFirst()
        }
    }

    // Shared navigation helper: focus previous item or cycle to header
    function navigateToPrevItem(currentIndex) {
        if (currentIndex > 0) {
            view.currentIndex = currentIndex - 1
            view.positionViewAtIndex(currentIndex - 1, ListView.Contain)
            Qt.callLater(function() {
                let prevItem = view.itemAtIndex(currentIndex - 1)
                if (prevItem && prevItem.enabled) {
                    // Focus on the item itself first, not directly to buttons
                    prevItem.forceActiveFocus()
                }
            })
        } else {
            view.gotoHeaderLast()
        }
    }

    DelegateChoice {
        roleValue: "group"
        GroupNotify {
            id: groupNotify
            objectName: "group-" + model.appName
            width: NotifyStyle.contentItem.width
            appName: model.appName
            activeFocusOnTab: false
            z: index

            onGotoNextItem: root.navigateToNextItem(index)
            onGotoPrevItem: root.navigateToPrevItem(index)

            onCollapse: function () {
                console.log("collapse group", model.appName)
                let collapseIndex = index
                notifyModel.collapseApp(index)
                root.view.requestFocusOnExpand(collapseIndex)
            }

            onSetting: function (pos) {
                let tmp = mapToItem(root.view, pos)
                root.setting(tmp, {
                                 appName: model.appName,
                                 pinned: model.pinned
                             })
            }
            onRemove: function () {
                console.log("remove group", model.appName)
                notifyModel.removeByApp(model.appName)
            }
        }
    }

    DelegateChoice {
        roleValue: "normal"
        NormalNotify {
            id: normalNotify
            objectName: "normal-" + model.appName
            width: NotifyStyle.contentItem.width
            activeFocusOnTab: false
            z: index

            onGotoNextItem: root.navigateToNextItem(index)
            onGotoPrevItem: root.navigateToPrevItem(index)

            appName: model.appName
            iconName: model.iconName
            date: model.time
            actions: model.actions
            title: model.title
            content: model.content
            strongInteractive: model.strongInteractive
            contentIcon: model.contentIcon
            contentRowCount: model.contentRowCount
            defaultAction: model.defaultAction
            indexInGroup: model.indexInGroup

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: function (eventPoint, button) {
                    let pos = eventPoint.position
                    setting(pos)
                }
            }

            onSetting: function (pos) {
                let tmp = mapToItem(root.view, pos)
                root.setting(tmp, {
                                 appName: model.appName,
                                 pinned: model.pinned
                             })
            }
            onRemove: function () {
                console.log("remove normal", model.id)
                let removeIndex = index
                notifyModel.remove(model.id)
                root.view.nextIndex = Math.min(removeIndex, root.view.count - 1)
            }
            onDismiss: function () {
                console.log("dismiss normal", model.id)
                let dismissIndex = index
                notifyModel.remove(model.id)
                root.view.nextIndex = Math.min(dismissIndex, root.view.count - 1)
            }
            onActionInvoked: function (actionId) {
                console.log("action normal", model.id, actionId)
                notifyModel.invokeAction(model.id, actionId)
            }
        }
    }

    DelegateChoice {
        roleValue: "overlap"
        OverlapNotify {
            id: overlapNotify
            objectName: "overlap-" + model.appName
            width: NotifyStyle.contentItem.width
            activeFocusOnTab: false
            z: index

            onGotoNextItem: root.navigateToNextItem(index)
            onGotoPrevItem: root.navigateToPrevItem(index)

            count: model.overlapCount
            appName: model.appName
            iconName: model.iconName
            date: model.time
            actions: model.actions
            title: model.title
            content: model.content
            strongInteractive: model.strongInteractive
            contentIcon: model.contentIcon
            contentRowCount: model.contentRowCount
            enableDismissed: false
            notifyContent.clearButton: AnimationSettingButton {
                id: clearBtn
                icon.name: "clean-alone"
                text: qsTr("Clean All")
                activeFocusOnTab: false
                focusBorderVisible: activeFocus
                Keys.onTabPressed: function(event) {
                    // Try to focus first action button
                    if (overlapNotify.focusFirstButton()) {
                        event.accepted = true
                        return
                    }
                    // No enabled action buttons, go to next item
                    overlapNotify.gotoNextItem()
                    event.accepted = true
                }
                Keys.onBacktabPressed: function(event) {
                    // Shift+Tab: go back to previous item (clear button is first in tab order)
                    overlapNotify.gotoPrevItem()
                    event.accepted = true
                }
                onClicked: function () {
                    notifyContent.remove()
                }
            }

            Component.onCompleted: {
                // Pass clear button reference to OverlapNotify
                overlapNotify.clearButton = clearBtn
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: function (eventPoint, button) {
                    let pos = eventPoint.position
                    setting(pos)
                }
            }

            onExpand: function ()
            {
                console.log("expand")
                let expandIndex = model.index
                notifyModel.expandApp(expandIndex)
                root.view.requestFocusOnExpand(expandIndex + 1)
            }
            onSetting: function (pos) {
                let tmp = mapToItem(root.view, pos)
                root.setting(tmp, {
                                 appName: model.appName,
                                 pinned: model.pinned
                             })
            }
            onRemove: function () {
                console.log("remove overlap", model.appName)
                notifyModel.removeByApp(model.appName)
            }
            onActionInvoked: function (actionId) {
                console.log("action overlap", model.id, actionId)
                notifyModel.invokeAction(model.id, actionId)
            }
        }
    }
}
