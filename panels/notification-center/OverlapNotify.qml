// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root

    property int count: 1
    readonly property int overlapItemRadius: 12

    contentItem: Item {
        width: parent.width
        implicitHeight: normalNotify.height + indicator.height
        NormalNotify {
            id: normalNotify
            width: parent.width
            appName: root.appName
            iconName: root.iconName
            content: root.content
            title: root.title
            date: root.date
            actions: root.actions
            closeVisible: root.hovered || root.activeFocus
            strongInteractive: root.strongInteractive
            contentIcon: root.contentIcon

            onRemove: function () {
                root.remove()
            }
            onActionInvoked: function (actionId) {
                root.actionInvoked(actionId)
            }
        }

        OverlapIndicator {
            id: indicator
            anchors {
                top: normalNotify.bottom
                left: parent.left
                leftMargin: overlapItemRadius
                right: parent.right
                rightMargin: overlapItemRadius
            }
            z: -1
            count: root.count
        }
    }

    component OverlapIndicator: Control {
        id: overlap

        required property var count

        contentItem: ColumnLayout {
            spacing: 0
            Repeater {
                model: overlap.count

                delegate: NotifyItemBackground {
                    Layout.topMargin: -overlapItemRadius
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: overlap.width - (index) * overlapItemRadius *2
                    Layout.preferredHeight: overlapItemRadius * 2
                    radius: overlapItemRadius
                    z: -index
                }
            }
        }

        background: BoundingRectangle {}
    }
}
