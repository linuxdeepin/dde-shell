// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    implicitWidth: 360
    implicitHeight: view.height

    NotifyStagingModel {
        id: notifyModel
        Component.onCompleted: {
            notifyModel.open()
        }
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
            title: model.title
            content: model.content
            strongInteractive: model.strongInteractive
            contentIcon: model.contentIcon

            onRemove: function () {
                console.log("remove overlap", model.id)
                notifyModel.closeNotify(model.id)
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

