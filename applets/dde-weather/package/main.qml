// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0

AppletItem {
    id: root
    objectName: "weather-applet"
    implicitWidth: 200
    implicitHeight: 100

    property bool expanded: false

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.1)

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 4

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: Applet.weatherIcon || "☀"
                font.pixelSize: 32
                color: "white"
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: Applet.temperature + "°C"
                font.pixelSize: 16
                font.bold: true
                color: "white"
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: Applet.currentCity || qsTr("Loading...")
                font.pixelSize: 11
                color: Qt.rgba(1, 1, 1, 0.7)
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (popup.visible) {
                    popup.close()
                } else {
                    popup.open()
                }
            }
        }
    }

    PanelPopup {
        id: popup
        x: -50
        y: -320
        width: 300
        height: 300

        WeatherDetail {
            anchors.fill: parent
        }
    }

    PanelToolTip {
        id: toolTip
        text: Applet.currentCity + " " + Applet.weatherDesc + " " + Applet.temperature + "°C"
        visible: false
    }

    Component.onCompleted: {
        Applet.refresh()
    }
}
