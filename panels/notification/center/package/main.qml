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

Window {
    id: root

    function windowMargin(position) {
        let dockApplet = DS.applet("org.deepin.ds.dock")
        if (!dockApplet)
            return 0

        let dockScreen = dockApplet.screenName
        let screen = root.screen.name
        let dockHideState = dockApplet.hideState
        let dockIsHide = dockHideState === 2
        if (dockScreen !== screen || dockIsHide)
            return 0

        let dockSize = dockApplet.dockSize
        let dockPosition = dockApplet.position
        return dockPosition === position ? dockSize : 0
    }

    // visible: true
    visible: Panel.visible
    flags: Qt.Tool

    property int contentPadding: 20
    width: NotifyStyle.contentItem.width + contentPadding * 2
    // height: 800
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom
    DLayerShellWindow.topMargin: windowMargin(0)
    DLayerShellWindow.rightMargin: windowMargin(1)
    DLayerShellWindow.bottomMargin: windowMargin(2)
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
