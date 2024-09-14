// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

Control {
    id: root
    focus: true

    required property NotifyModel notifyModel
    readonly property real viewHeight: view.contentHeight

    NotifySetting {
        id: notifySetting
        notifyModel: root.notifyModel
    }

    contentItem: ListView {
        id: view
        spacing: 10
        // activeFocusOnTab: true
        ScrollBar.vertical: ScrollBar { }

        model: root.notifyModel
        delegate: NotifyViewDelegate {
            id: notifyDelegate
            notifyModel: root.notifyModel
            view: view
            onSetting: function (pos, params) {
                let appName = params.appName
                let pinned = params.pinned
                notifySetting.x = pos.x - notifySetting.width / 2
                notifySetting.y = pos.y
                notifySetting.appName = appName
                notifySetting.pinned = pinned

                console.log("setting", appName, pinned)
                notifySetting.toggle();
            }
        }
        // remove: Transition {
        //     ParallelAnimation {
        //         NumberAnimation { properties: "y"; duration: 300 }
        //     }
        // }
        // add: Transition {
        //     ParallelAnimation {
        //         NumberAnimation { properties: "y"; duration: 300 }
        //     }
        // }
        addDisplaced: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "y"; duration: 300 }
            }
        }
        removeDisplaced: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "y"; duration: 300 }
            }
        }
        moveDisplaced: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "y"; duration: 300 }
            }
        }
    }

    background: BoundingRectangle {}
}
