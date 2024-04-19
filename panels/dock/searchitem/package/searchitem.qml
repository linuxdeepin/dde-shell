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
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30 
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize
    property bool shouldVisible: Applet.visible

    PanelToolTip {
        id: toolTip
        text: qsTr("search")
    }

    D.ToolButton {
        anchors.centerIn: parent
        width: 30
        height: 30
        icon.name: "search"
        icon.width: 16
        icon.height: 16
        onClicked: {
            toolTip.close()
            Applet.toggleGrandSearch()
        }
        onHoveredChanged: {
            if (hovered) {
                var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
                toolTip.toolTipX = point.x
                toolTip.toolTipY = point.y
                toolTip.open()
            } else {
                toolTip.close()
            }
        }
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
            text: qsTr("SearchConfig")
            onTriggered: {
                Applet.toggleGrandSearchConfig()
            }
        }
    }
}
