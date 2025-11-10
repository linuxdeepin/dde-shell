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
            z: index

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
            z: index

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
            activeFocusOnTab: true
            z: index

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
                icon.name: "clean-alone"
                text: qsTr("Clean All")
                onClicked: function () {
                    notifyContent.remove()
                }
            }

            Loader {
                anchors.fill: parent
                active: overlapNotify.activeFocus && NotifyAccessor.debugging

                sourceComponent: FocusBoxBorder {
                    radius: overlapNotify.overlapItemRadius
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
                let expandIndex = model.index
                notifyModel.expandApp(expandIndex)
                root.view.nextIndex = expandIndex + 1
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
