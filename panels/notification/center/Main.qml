// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
// import org.deepin.ds 1.0
import org.deepin.ds.notificationcenter

Window {
    id: root

    visible: true

    width: 380
    height: 800
    // height: Screen.desktopAvailableHeight
    // DLayerShellWindow.anchors: DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom

    ColorSelector.family: Palette.CrystalColor
    DWindow.enabled: true
    DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    // DWindow.enableSystemResize: false
    // DWindow.enableSystemMove: false
    // color: "transparent"
    DWindow.enableBlurWindow: true

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
