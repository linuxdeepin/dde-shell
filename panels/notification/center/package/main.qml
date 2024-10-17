// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds 1.0
import org.deepin.ds.notificationcenter

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

    // visible: true
    visible: Panel.visible
    flags: Qt.Tool

    width: 360 + view.anchors.leftMargin + view.anchors.rightMargin
    // height: 800
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom
    DLayerShellWindow.topMargin: windowMargin(0) + 10
    DLayerShellWindow.rightMargin: windowMargin(1) + 10
    DLayerShellWindow.bottomMargin: windowMargin(2) + 10
    ColorSelector.family: Palette.CrystalColor
    DWindow.enabled: true
    DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    DWindow.windowRadius: DTK.platformTheme.windowRadius
    // DWindow.enableSystemResize: false
    // DWindow.enableSystemMove: false
    color: "transparent"
    DWindow.enableBlurWindow: true
    screen: Qt.application.screens[0]
    // TODO `Qt.application.screens[0]` maybe invalid, why screen is changed.
    onScreenChanged: {
        root.screen = Qt.binding(function () { return Qt.application.screens[0]})
    }

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

    // close Panel when click dock.
    Connections {
        target: DS.applet("org.deepin.ds.dock")
        function onRequestClosePopup() {
            Panel.close()
        }
    }

    ColumnLayout {
        id: view
        width: parent.width
        anchors {
            top: parent.top
            left: parent.left
            margins: 10
        }

        NotifyStaging {
            id: notifyStaging
            implicitWidth: 360

        }

        NotifyCenter {
            id: notifyCenter
            Connections {
                target: Panel
                function onVisibleChanged() {
                    if (!Panel.visible) {
                        notifyCenter.model.collapseAllApp()
                    }
                }
            }

            implicitWidth: 360
            maxViewHeight: root.height
        }
    }
}
