// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.15

import org.deepin.ds 1.0

AppletItem {
    objectName: "appplet data"
    implicitWidth: 200
    implicitHeight: 100
    Rectangle {
        anchors.fill: parent
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: Applet.mainText
        }
    }
}
