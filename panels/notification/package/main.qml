// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

Window {
    id: root
    visible: Applet.visible
    width: 600
    height: Math.max(10, bubbleView.height)
    DLayerShellWindow.topMargin: 10
    DLayerShellWindow.leftMargin: 800
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorTop

    ListView  {
        id: bubbleView
        width: root.width
        height: contentHeight
        spacing: 10
        model: Applet.bubbles
        interactive: false

        add: Transition {
            NumberAnimation { properties: "y"; from: -100; duration: 500 }
        }
        delegate: Bubble {
            bubble: model
        }
    }
    color: "transparent"
}
