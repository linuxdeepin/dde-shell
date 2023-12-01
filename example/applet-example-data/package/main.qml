// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import Qt.labs.platform 1.1 as LP

AppletItem {
    objectName: "appplet data"
    implicitWidth: 200
    implicitHeight: 100
    Rectangle {
        anchors.fill: parent
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: Applet.mainText + "\n" +
                  String("this AppletId:%1 \n").arg(String(Applet.id).slice(1, 10)) +
                  String("parent AppletId:%2 \n").arg(String(Applet.parent.id).slice(1, 10))
        }
        MouseArea {
            anchors.fill: parent
            onClicked: platformMenu.open()
        }
    }
    LP.Menu {
        id: platformMenu
        LP.MenuItem {
            text: "item 1"
        }
    }
}
