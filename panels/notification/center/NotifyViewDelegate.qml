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

    DelegateChoice {
        roleValue: "group"
        GroupNotify {
            id: groupNotify
            objectName: "group-" + model.appName
            width: NotifyStyle.contentItem.width
            appName: model.appName
            activeFocusOnTab: true

            Loader {
                anchors.fill: parent
                active: groupNotify.activeFocus && NotifyAccessor.debugging

                sourceComponent: FocusBoxBorder {
                    radius: groupNotify.radius
                    color: groupNotify.palette.highlight
                }
            }

            onCollapse: function () {
                console.log("collapse group", model.appName)
                notifyModel.collapseApp(index)
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
            activeFocusOnTab: true

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

            Loader {
                anchors.fill: parent
                active: normalNotify.activeFocus && NotifyAccessor.debugging

                sourceComponent: FocusBoxBorder {
                    radius: normalNotify.radius
                    color: normalNotify.palette.highlight
                }
            }

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
                notifyModel.remove(model.id)
            }
            onDismiss: function () {
                console.log("dismiss normal", model.id)
                notifyModel.remove(model.id)
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
            activeFocusOnTab: true

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

            Loader {
                anchors.fill: parent
                active: overlapNotify.activeFocus && NotifyAccessor.debugging

                sourceComponent: FocusBoxBorder {
                    radius: overlapNotify.radius
                    color: overlapNotify.palette.highlight
                }
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
                notifyModel.expandApp(model.index)
            }
            onSetting: function (pos) {
                let tmp = mapToItem(root.view, pos)
                root.setting(tmp, {
                                 appName: model.appName,
                                 pinned: model.pinned
                             })
            }
            onRemove: function () {
                console.log("remove overlap", model.id)
                notifyModel.remove(model.id)
            }
            onActionInvoked: function (actionId) {
                console.log("action overlap", model.id, actionId)
                notifyModel.invokeAction(model.id, actionId)
            }
        }
    }
}
