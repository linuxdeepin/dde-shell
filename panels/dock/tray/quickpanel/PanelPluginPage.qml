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

    Flow {
        id: pluginView
        width: 310
        height: childrenRect.height
        spacing: 10
        Repeater {
            anchors.fill: parent
            model: viewModel
        }
        RowLayout {
            width: pluginView.width
            ActionButton {
                icon.name: "settings"
                onClicked: {
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

            ActionButton {
                icon.name: "settings"
                onClicked: {
                }
            }
        }
    }

    DelegateModel {
        id: viewModel
        model: root.model

        delegate: PluginItem {
            width: {
                if (model.surfaceLayoutType === 4)
                    return 310
                if (model.surfaceLayoutType === 2)
                    return 150
                return 70
            }
            height: 60
            shellSurface: model.surface
            itemKey: model.surfaceItemKey
            traySurface: model.traySurface
        }
    }
}
