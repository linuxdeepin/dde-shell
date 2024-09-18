// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds 1.0
import org.deepin.ds.notificationcenter

Window {
    id: root

    function windowMargin(position) {
        let dockApplet = DS.applet("org.deepin.ds.dock")
        if (!dockApplet)
            return 0

        let dockSize = dockApplet.dockSize
        let dockPosition = dockApplet.position
        return dockPosition === position ? dockSize + 10 : 0
    }

    visible: Panel.visible
    flags: Qt.Tool

    width: 380
    // height: 800
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom
    DLayerShellWindow.topMargin: windowMargin(0)
    DLayerShellWindow.rightMargin: windowMargin(1)
    DLayerShellWindow.bottomMargin: windowMargin(2)
    // DLayerShellWindow.bottomMargin: 60
    ColorSelector.family: Palette.CrystalColor
    DWindow.enabled: true
    DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    DWindow.windowRadius: DTK.platformTheme.windowRadius
    // DWindow.enableSystemResize: false
    // DWindow.enableSystemMove: false
    color: "transparent"
    DWindow.enableBlurWindow: true

    onVisibleChanged: function (v) {
        if (v) {
            requestActivate()
        }
    }

    onActiveChanged: function () {
        if (!root.active) {
            Panel.close()
        }
    }

    NotifyCenter {
        id: notifyCenter
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 10
        }

        width: 360
        height: root.height
        maxViewHeight: root.height
    }
}
