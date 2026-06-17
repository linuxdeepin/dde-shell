// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0

Rectangle {
    id: detail
    color: Qt.rgba(0.15, 0.15, 0.15, 0.95)
    radius: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: Applet.weatherIcon || "☀"
                font.pixelSize: 48
                color: "white"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: Applet.currentCity || qsTr("Unknown")
                    font.pixelSize: 18
                    font.bold: true
                    color: "white"
                }

                Text {
                    text: Applet.temperature + "°C"
                    font.pixelSize: 28
                    font.bold: true
                    color: "white"
                }

                Text {
                    text: Applet.weatherDesc || ""
                    font.pixelSize: 14
                    color: Qt.rgba(1, 1, 1, 0.7)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Qt.rgba(1, 1, 1, 0.1)
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: Applet.model
            spacing: 8
            clip: true

            delegate: RowLayout {
                width: ListView.view.width
                spacing: 12

                Text {
                    text: model.icon || "☀"
                    font.pixelSize: 20
                    color: "white"
                }

                Text {
                    text: model.city || ""
                    font.pixelSize: 14
                    color: "white"
                    Layout.fillWidth: true
                }

                Text {
                    text: model.temperature + "°C"
                    font.pixelSize: 14
                    font.bold: true
                    color: "white"
                }

                Text {
                    text: model.description || ""
                    font.pixelSize: 12
                    color: Qt.rgba(1, 1, 1, 0.6)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("Refresh")
                onClicked: Applet.refresh()
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Last update: --:--")
                font.pixelSize: 11
                color: Qt.rgba(1, 1, 1, 0.5)
            }
        }
    }
}
