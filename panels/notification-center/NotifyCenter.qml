// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    property alias model: notifyModel
    property int maxViewHeight: 400

    NotifyModel {
        id: notifyModel
    }
    Component.onCompleted: {
        notifyModel.open()
    }

    Item {
        objectName: "notificationCenter"
        anchors.fill: parent
        NotifyHeader {
            id: header
            height: 40
            width: parent.width
            notifyModel: notifyModel
        }

        NotifyView {
            id: view
            anchors {
                left: parent.left
                top: header.bottom
                topMargin: 10
            }

            width: parent.width
            height: Math.min(maxViewHeight, viewHeight)
            notifyModel: notifyModel
        }

        NotifyBottom {
            id: bottom
            anchors {
                top: view.bottom
                topMargin: 20
                horizontalCenter: parent.horizontalCenter
            }

            visible: notifyModel.collapse && notifyModel.remainCount > 0
            notifyModel: notifyModel
        }
    }
}

