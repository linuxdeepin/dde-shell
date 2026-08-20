// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: control
    implicitWidth: 338
    implicitHeight: 60

    readonly property bool singleView: true

    function update(osdType)
    {
        if (match(osdType)) {
            Applet.sync()
            return true
        }
        return false
    }

    function match(osdType)
    {
        return types.indexOf(osdType) >= 0
    }

    property var types: [
        "AudioUp",
        "AudioDown",
        "AudioMute",
        "AudioUpAsh",
        "AudioDownAsh",
        "AudioMuteAsh"
    ]

    RowLayout {
        spacing: 0
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        anchors.topMargin: 2

        D.DciIcon {
            width: 32
            height: 32
            sourceSize {
                width: 32
                height: 32
            }
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.leftMargin: 14
            name: Applet.iconName
            theme: D.DTK.themeType
            palette: D.DTK.makeIconPalette(control.palette)
        }

        ColumnLayout {
            spacing: 0
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.leftMargin: 14
            Layout.preferredWidth: 220
            Layout.fillHeight: true

            ScaleCom {
                visible: Applet.increaseVolume
                Layout.bottomMargin: 2
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.leftMargin: progressBar.width / 1.5
            }

            D.ProgressBar {
                id: progressBar
                Layout.preferredHeight: 4
                Layout.preferredWidth: 220
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                from: 0
                value: Applet.volumeValue / (Applet.increaseVolume ? 1.5 : 1.0)
                to: 1

                property D.Palette contentColor: D.Palette {
                    normal: Qt.rgba(0, 0, 0, 1)
                    normalDark: Qt.rgba(1, 1, 1, 1)
                }
                property D.Palette backgroundColor: D.Palette {
                    normal: Qt.rgba(0, 0, 0, 0.1)
                    normalDark: Qt.rgba(1, 1, 1, 0.1)
                }
                contentItem: Item {
                    width: progressBar.visualPosition * parent.width
                    height: parent.height
                    Rectangle {
                        anchors.fill: parent
                        radius: 2
                        color: progressBar.D.ColorSelector.contentColor
                    }
                    Behavior on width {
                        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }
                }
                background: Rectangle {
                    color: progressBar.D.ColorSelector.backgroundColor
                    radius: 2
                }
            }

            ScaleCom {
                visible: Applet.increaseVolume
                Layout.topMargin: 2
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.leftMargin: progressBar.width / 1.5
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            font: D.DTK.fontManager.t4
            text: Number(Applet.volumeValue * 100).toFixed(0)
            color: palette.windowText
        }
        Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.topMargin: height / 2
            Layout.rightMargin: 14
            font: D.DTK.fontManager.t10
            color: palette.windowText
            text: "%"
        }
    }

    component ScaleCom: Control {
        id: scaleControl
        property D.Palette scaleColor: D.Palette {
            normal: Qt.rgba(0, 0, 0, 0.3)
            normalDark: Qt.rgba(1, 1, 1, 0.3)
        }

        contentItem: Rectangle {
            implicitHeight: 6
            implicitWidth: 2
            color: scaleControl.D.ColorSelector.scaleColor
        }
    }
}
