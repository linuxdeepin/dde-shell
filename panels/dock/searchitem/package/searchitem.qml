// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

import Qt.labs.platform 1.1 as LP

AppletItem {
    id: searchItem
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 3
    implicitWidth: dockSize
    implicitHeight: dockSize

    D.ActionButton {
        anchors.fill: parent
        icon.name: "search"
        onClicked: Applet.toggleGrandSearch()
    }

    MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: platformMenu.open()
        }

    LP.Menu {
        id: platformMenu
        LP.MenuItem {
            text: "SearchConfig"
            onTriggered: {
                Applet.toggleGrandSearchConfig()
            }
        }
    }
}
