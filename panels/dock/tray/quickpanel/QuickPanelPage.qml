// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.dtk 1.0

Item {
    id: root
    width: panelView.width
    height: panelView.height

    required property var model

    Connections {
        target: model
        function onRequestShowSubPlugin(pluginId, surface) {
            console.log("show subplugin, plugin", pluginId)
            panelView.push(subPluginPageLoader,
                           {
                               pluginId: pluginId,
                               model: root.model,
                               shellSurface: surface
                           },
                           StackView.PushTransition)
        }
    }

    StackView {
        id: panelView
        width: panelPage.width
        height: panelPage.childrenRect.height
        initialItem: PanelPluginPage {
            id: panelPage
            model: root.model
        }
    }

    Component {
        id: subPluginPageLoader
        SubPluginPage {
            onRequestBack: function () {
                panelView.pop()
            }
        }
    }
}
