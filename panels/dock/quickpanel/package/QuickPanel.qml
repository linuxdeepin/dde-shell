// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.ds 1.0
import org.deepin.dtk 1.0

Item {
    id: root
    width: panelView.width
    height: panelView.height

    required property var model

    StackView {
        id: panelView
        width: panelPage.width
        height: panelPage.childrenRect.height
        initialItem: PanelPluginPage {
            id: panelPage
            model: root.model
            onRequestShowSubPlugin: function (plugin) {
                panelView.push(subPluginPageLoader,
                               {
                                   pluginKey: plugin,
                                   model: root.model
                               },
                               StackView.PushTransition)
            }
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
