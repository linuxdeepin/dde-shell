// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    palette: DTK.palette

    property alias model: notifyModel
    property alias viewPanelShown: view.viewPanelShown
    property int maxViewHeight: 400
    property int stagingViewCount: 0

    NotifyModel {
        id: notifyModel
    }

    Item {
        objectName: "notificationCenter"
        anchors.fill: parent
        NotifyHeader {
            id: header
            anchors {
                top: parent.top
                left: parent.left
            }
            height: 40
            width: NotifyStyle.contentItem.width
            notifyModel: notifyModel
            z: 1
        }

        NotifyView {
            id: view
            anchors {
                left: parent.left
                top: header.bottom
                right: parent.right
                rightMargin: NotifyStyle.scrollBarPadding
                topMargin: 10
                bottom: parent.bottom
            }

            height: Math.min(maxViewHeight, viewHeight)
            notifyModel: notifyModel
        }

        DropShadowText {
            text: qsTr("No recent notifications")
            visible: root.stagingViewCount === 0 && view.viewCount === 0
            anchors {
                top: header.bottom
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }
            height: 40
        }
    }
}
