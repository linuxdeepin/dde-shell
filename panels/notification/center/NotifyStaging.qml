// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    implicitWidth: 360
    implicitHeight: view.height
    property var model: notifyModel
    readonly property int viewCount: view.count

    NotifyStagingModel {
        id: notifyModel
    }

    ListView {
        id: view
        spacing: 10
        snapMode: ListView.SnapToItem
        // activeFocusOnTab: true
        width: root.width
        height: contentHeight

        model: notifyModel
        delegate: OverlapNotify {
            id: overlapNotify
            objectName: "overlap-" + model.appName
            focus: true
            width: 360
            activeFocusOnTab: true

            count: model.overlapCount
            appName: model.appName
            iconName: model.iconName
            date: model.time
            actions: model.actions
            defaultAction: model.defaultAction
            title: model.title
            content: model.content
            strongInteractive: model.strongInteractive
            contentIcon: model.contentIcon
            contentRowCount: model.contentRowCount

            clearButton: SettingActionButton {
                icon.name: "clean-alone"
                onClicked: function () {
                    overlapNotify.removedCallback = function() {
                        overlapNotify.remove()
                    }
                    overlapNotify.state = "removing"
                }
            }

            onRemove: function () {
                console.log("remove overlap", model.id)
                notifyModel.closeNotify(model.id, NotifyItem.Closed)
            }
            onDismiss: function () {
                console.log("dismiss overlap", model.id)
                notifyModel.closeNotify(model.id, NotifyItem.Dismissed)
            }
            onActionInvoked: function (actionId) {
                console.log("action overlap", model.id, actionId)
                notifyModel.invokeNotify(model.id, actionId)
            }
        }
        displaced: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "y"; duration: 300 }
            }
        }
    }
}

