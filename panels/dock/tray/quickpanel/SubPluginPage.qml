// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor

import org.deepin.dtk 1.0

Item {
    id: root
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height

    required property var pluginKey
    property alias shellSurface: surfaceLayer.shellSurface
    required property var model
    signal requestBack()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.topMargin: 10
            Layout.rightMargin: 12
            ActionButton {
                icon.name: "arrow_ordinary_left"
                icon.width: 16
                icon.height: 16
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                onClicked: {
                    console.log("request back:", pluginKey)
                    shellSurface.close()
                    requestBack()
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: model.getTitle(pluginKey)
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
        }

        Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }

        // content
        ShellSurfaceItem {
            id: surfaceLayer
            Layout.fillHeight: true
            Layout.fillWidth: true
            // Rectangle {
            //     anchors.fill: parent
            //     color: "red"
            //     opacity: 0.3
            // }
        }

        Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }
    }
}
