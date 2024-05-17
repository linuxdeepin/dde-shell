// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

import Qt.labs.platform 1.1 as LP

DockItem {
    id: searchItem
    dockOrder: 3
    shouldVisible: Applet.visible
    toolButtonColor: DockPalette.toolButtonColor
    toolButtonBorderColor: DockPalette.toolButtonBorderColor
    toolTip: qsTr("GrandSearch")
    icon: Applet.icon

    property int dockSize: Panel.rootObject.dockSize
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30 
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            platformMenuLoader.active = true
            platformMenuLoader.item.open()
        }
    }

    Loader {
        id: platformMenuLoader
        active: false
        sourceComponent: LP.Menu {
            id: platformMenu
            LP.MenuItem {
                text: qsTr("SearchConfig")
                onTriggered: {
                    Applet.toggleGrandSearchConfig()
                }
            }
        }
    }
}
