// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0

Window {
    id: root

    function windowMargin(position) {
        let dockApplet = DS.applet("org.deepin.ds.dock")
        if (!dockApplet)
            return 0

        let dockRect = dockApplet.frontendWindowRect
        let rect = Qt.rect(root.screen.virtualX, root.screen.virtualY, root.screen.width, root.screen.height)
        if (!containsPos(rect, Qt.point(dockRect.x, dockRect.y)))
            return 0

        let dockSize = dockApplet.dockSize
        let dockPosition = dockApplet.position
        return dockPosition === position ? dockSize : 0
    }

    function containsPos(rect1, pos) {
        if (rect1.x <= pos.x &&
                rect1.y <= pos.y &&
                (rect1.x + rect1.width > pos.x) &&
                (rect1.y + rect1.height > pos.y)) {
            return true
        }
        return false
    }

    visible: Applet.visible
    width: 380
    height: Math.max(10, bubbleView.height + bubbleView.anchors.topMargin + bubbleView.anchors.bottomMargin)
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorRight
    DLayerShellWindow.topMargin: windowMargin(0)
    DLayerShellWindow.rightMargin: windowMargin(1)
    DLayerShellWindow.bottomMargin: windowMargin(2)
    ColorSelector.family: Palette.CrystalColor
    DWindow.enabled: true
    DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    color: "transparent"
    // DWindow.enableBlurWindow: true
    screen: Qt.application.screens[0]
    // TODO `Qt.application.screens[0]` maybe invalid, why screen is changed.
    onScreenChanged: {
        root.screen = Qt.binding(function () { return Qt.application.screens[0]})
    }

    ListView  {
        id: bubbleView
        width: 360
        height: contentHeight
        anchors {
            centerIn: parent
            margins: 10
        }

        spacing: 10
        model: Applet.bubbles
        interactive: false
        verticalLayoutDirection: ListView.BottomToTop
        add: Transition {
            id: addTrans
            NumberAnimation { properties: "x"; from: addTrans.ViewTransition.item.width; duration: 600; easing.type: Easing.OutExpo }
        }
        delegate: Bubble {
            width: 360
            bubble: model
        }
    }
}
