// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import Qt.labs.platform 1.1 as LP

AppletItem {
    id: root
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
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: platformMenu.open()
        }
    }
    Binding {
        target: Applet
        property: "userData"
        value: true
        when: root.applet.rootObject
    }

    Component.onCompleted: {
        console.log("test before onCompleted", Applet.userData, root.applet)
        var appearance = DS.applet("org.deepin.ds.dde-appearance")
        console.log(appearance, appearance.opacity)
    }

    PanelPopup {
        Accessible.role: Accessible.Dialog
        Accessible.id: "Popup"
        id: popup
        x: 100
        y: -200
        width: 200
        height: 200

        Button {
            Accessible.id: "PopupContent"
            text: "popup content"
            anchors.centerIn: parent
        }
    }

    PanelToolTip {
        Accessible.role: Accessible.ToolTip
        Accessible.id: "TooltipContent"
        id: toolTip
        x: 100
        y: -100
        width: 100
        height: 100
        visible: mouseArea.containsMouse
        text: "toolTip content"
    }

    LP.Menu {
        Accessible.id: "PlatformMenu"
        id: platformMenu
        LP.MenuItem {
            Accessible.id: "Item1"
            text: "item 1"
        }
        LP.MenuItem {
            Accessible.id: "NonItem"
            text: "non Item " + String(Applet.id).slice(0, 5)
        }
        LP.MenuItem {
            Accessible.id: "Panel"
            text: "Panel " + String(Panel.id).slice(0, 5)
        }
        LP.MenuItem {
            Accessible.id: "DSApplet"
            text: "DS.applet " + String(DS.applet("org.deepin.ds.example").id).slice(0, 5)
        }
        LP.MenuItem {
            Accessible.id: "Popup"
            text: "Popup"
            onTriggered: {
                popup.open()
            }
        }
    }
}
