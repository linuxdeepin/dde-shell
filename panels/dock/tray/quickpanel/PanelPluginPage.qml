// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQml

import org.deepin.ds 1.0
import org.deepin.dtk 1.0

Item {
    id: root
    width: pluginView.width
    height: pluginView.height
    readonly property int cellSize: 70
    readonly property int columnCellCounts: 4
    required property var model
    readonly property int pluginViewHeight: pluginView.height
    function forceLayout()
    {
        ready()
    }
    signal ready()

    Flow {
        id: pluginView
        width: 330
        spacing: 10
        padding: 10
        // TODO it's only work once, incorrect position for later.
        onPositioningComplete: function () {
            if (root.visible)
                root.forceLayout()
        }
        Repeater {
            model: viewModel
        }

        RowLayout {
            width: pluginView.width - pluginView.leftPadding - pluginView.rightPadding
            SettingButton {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.leftMargin: 6
                icon.name: "quickpanel-setting"
                onClicked: function () {
                    console.log("clicked settings")
                    model.openSystemSettings()
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

            SettingButton {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.rightMargin: 6
                icon.name: "quickpanel-power"
                onClicked: function () {
                    console.log("clicked shutdown")
                    model.openShutdownScreen()
                }
            }
        }
    }

    DelegateModel {
        id: viewModel
        model: root.model

        delegate: PluginItem {
            id: pluginItem
            width: {
                if (model.surfaceLayoutType === 4)
                    return 310
                if (model.surfaceLayoutType === 2)
                    return 150
                return 70
            }
            height: 60
            shellSurface: model.surface
            pluginId: model.pluginId
            itemKey: model.traySurfaceItemKey
            traySurface: model.traySurface

            Connections {
                target: root
                function onReady () {
                    pluginItem.updateSurface()
                }
            }
        }
    }

    component SettingButton: ActionButton {
        id: btn
        focusPolicy: Qt.NoFocus
        palette.windowText: ColorSelector.textColor
        icon {
            width: 16
            height: 16
        }
    }
}
