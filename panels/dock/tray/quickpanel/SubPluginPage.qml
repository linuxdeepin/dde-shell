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
    implicitWidth: 330
    height: Math.min(Math.max(subPluginMinHeight, childrenRect.height), 600)

    required property var pluginId
    property alias shellSurface: surfaceLayer.shellSurface
    required property var model
    signal requestBack()
    property int subPluginMinHeight

    Component.onCompleted: {
        var surfaceMinHeight = subPluginMinHeight - titleLayer.height
        shellSurface.setEmbedPanelMinHeight(surfaceMinHeight)
    }

    ColumnLayout {
        spacing: 0
        width: root.width

        // header
        RowLayout {
            id: titleLayer
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
                    console.log("request back:", pluginId)
                    shellSurface.close()
                    requestBack()
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: model.getTitle(pluginId)
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
        }

        Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }

        // content
        ShellSurfaceItemProxy {
            id: surfaceLayer
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true; Layout.preferredWidth: 1 }
    }
}
