// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

import org.deepin.ds 1.0
import org.deepin.dtk 1.0

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

        let dockSize = dockApplet.rootObject.dockSize
        let dockPosition = dockApplet.position
        return dockPosition === position ? dockSize : 0
    }

    visible: Applet.visible
    width: 390
    height: Math.max(10, bubbleView.height + bubbleView.anchors.topMargin + bubbleView.anchors.bottomMargin)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorRight
    DLayerShellWindow.topMargin: windowMargin(0)
    DLayerShellWindow.rightMargin: windowMargin(1)
    DLayerShellWindow.bottomMargin: windowMargin(2)
    DLayerShellWindow.exclusionZone: -1
    palette: DTK.palette
    ColorSelector.family: Palette.CrystalColor
    // DWindow.windowEffect: PlatformHandle.EffectNoBorder | PlatformHandle.EffectNoShadow
    color: "transparent"
    // DWindow.windowRadius: 0
    // DWindow.enabled: false
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
            right: parent.right
            bottom: parent.bottom
            bottomMargin: 10
            rightMargin: 10
            margins: 30
        }

        spacing: 10
        model: Applet.bubbles
        interactive: false
        verticalLayoutDirection: ListView.BottomToTop
        add: Transition {
            id: addTrans
            // Before starting the new animation, forcibly complete the previous notification bubble's animation
            ScriptAction {
                script: {
                    // Only handle the previous notification bubble (at index count - 1); no need to iterate through all of them
                    if (bubbleView.count > 1) {
                        let prevItem = bubbleView.itemAtIndex(bubbleView.count - 2)
                        if (prevItem) {
                            // Directly set x to 0 to forcibly complete the animation
                            prevItem.x = 0
                        }
                    }
                }
            }
            XAnimator { 
                target: addTrans.ViewTransition.item
                from: addTrans.ViewTransition.item.width
                to: 0
                duration: 600
                easing.type: Easing.OutExpo
            }
        }
        delegate: Bubble {
            width: 360
            bubble: model
        }
    }
}
