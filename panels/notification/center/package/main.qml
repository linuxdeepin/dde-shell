// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter
import org.deepin.ds.dock 1.0

Window {
    id: root

    property int topMarginValue: 0
    property int rightMarginValue: 0
    property int bottomMarginValue: 0

    function updateMargins(frontendRect) {
        let dockApplet = DS.applet("org.deepin.ds.dock")
        if (!dockApplet || !root.screen) {
            topMarginValue = 0
            rightMarginValue = 0
            bottomMarginValue = 0
            return
        }

        let dockScreen = dockApplet.screenName
        let screen = root.screen ? root.screen.name : ""

        if (dockScreen !== screen) {
            topMarginValue = 0
            rightMarginValue = 0
            bottomMarginValue = 0
            return
        }

        if (!frontendRect) {
            frontendRect = dockApplet.frontendWindowRect
        }

        if (!frontendRect) {
            topMarginValue = 0
            rightMarginValue = 0
            bottomMarginValue = 0
            return
        }

        // frontendRect is already in device pixels from dockpanel.cpp
        // We need to convert it back to logical pixels
        let dpr = root.screen.devicePixelRatio
        let dockGeometry = Qt.rect(
            frontendRect.x / dpr,
            frontendRect.y / dpr,
            frontendRect.width / dpr,
            frontendRect.height / dpr
        )

        // Get screen geometry in logical pixels
        let screenGeometry = Qt.rect(
            root.screen.virtualX,
            root.screen.virtualY,
            root.screen.width,
            root.screen.height
        )

        let newTopMargin = 0
        let newRightMargin = 0
        let newBottomMargin = 0

        if (dockGeometry.width > 0 && dockGeometry.height > 0) {
            let dockPosition = dockApplet.position
            switch (dockPosition) {
                case 0: { // DOCK_TOP
                    let visibleHeight = Math.max(0, dockGeometry.y + dockGeometry.height - screenGeometry.y)
                    newTopMargin = visibleHeight
                    break
                }
                case 1: { // DOCK_RIGHT
                    let visibleWidth = Math.max(0, screenGeometry.x + screenGeometry.width - dockGeometry.x)
                    newRightMargin = visibleWidth
                    break
                }
                //特殊处理:因为会上述计算会导致抖动
                case 2: { // DOCK_BOTTOM
                    let hideState = dockApplet.hideState
                    if (hideState === Dock.Hide) {
                        newBottomMargin = 0
                    } else {
                        newBottomMargin = dockApplet.dockSize
                    }
                    break
                }
            }
        }

        topMarginValue = newTopMargin
        rightMarginValue = newRightMargin
        bottomMarginValue = newBottomMargin
        
    }

    // visible: true
    visible: Panel.visible
    flags: Qt.Tool

    property int contentPadding: 20
    width: NotifyStyle.contentItem.width + contentPadding * 2
    // height: 800
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom
    DLayerShellWindow.topMargin: topMarginValue
    DLayerShellWindow.rightMargin: rightMarginValue
    DLayerShellWindow.bottomMargin: bottomMarginValue
    DLayerShellWindow.exclusionZone: -1
    DLayerShellWindow.keyboardInteractivity: DLayerShellWindow.KeyboardInteractivityOnDemand
    palette: DTK.palette
    ColorSelector.family: Palette.CrystalColor
    // DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    // DWindow.windowRadius: DTK.platformTheme.windowRadius
    // DWindow.enableSystemResize: false
    // DWindow.enableSystemMove: false
    // DWindow.windowRadius: 0
    // DWindow.enabled: false
    color: "transparent"
    // DWindow.enableBlurWindow: true
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
        function onFrontendWindowRectChanged(frontendWindowRect) {
            root.updateMargins(frontendWindowRect)
        }
    }

    Item {
        id: view
        // clear focus when NotificationCenter is closed.
        focus: root.visible
        anchors {
            top: parent.top
            topMargin: contentPadding
            left: parent.left
            leftMargin: contentPadding
            right: parent.right
            bottom: parent.bottom
        }

        NotifyStaging {
            id: notifyStaging
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                rightMargin: contentPadding
            }
            Connections {
                target: Panel
                function onVisibleChanged() {
                    if (Panel.visible) {
                        notifyStaging.model.open()
                        DS.singleShot(100, function() {
                            notifyCenter.viewPanelShown = true
                        })
                    } else {
                        notifyStaging.model.close()
                        notifyCenter.viewPanelShown = false
                    }
                }
            }
        }

        NotifyCenter {
            id: notifyCenter
            anchors {
                top: notifyStaging.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Connections {
                target: Panel
                function onVisibleChanged() {
                    if (Panel.visible) {
                        notifyCenter.model.open()
                        DS.singleShot(100, function() {
                            notifyCenter.viewPanelShown = true
                        })
                    } else {
                        notifyCenter.model.close()
                        notifyCenter.viewPanelShown = false
                    }
                }
            }

            maxViewHeight: root.height
            stagingViewCount: notifyStaging.viewCount
        }
    }
}
