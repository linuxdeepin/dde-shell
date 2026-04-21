// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
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
        if (dockScreen !== screen)
            return 0

        let dockPosition = dockApplet.position
        if (dockPosition !== position)
            return 0

        // Use frontendWindowRect to calculate the actual visible dock area,
        // so the margin accurately tracks dock position during hide/show animations
        let frontendRect = dockApplet.frontendWindowRect
        let dpr = root.screen.devicePixelRatio
        let dockGeometry = Qt.rect(
            frontendRect.x / dpr,
            frontendRect.y / dpr,
            frontendRect.width / dpr,
            frontendRect.height / dpr
        )

        let screenGeometry = Qt.rect(
            root.screen.virtualX,
            root.screen.virtualY,
            root.screen.width,
            root.screen.height
        )

        // Cap margin at dockSize to prevent incorrect values during position transitions
        // (when frontendWindowRect may temporarily have wrong dimensions because
        // position() changes immediately but window geometry hasn't updated yet)
        let dockSize = dockApplet.dockSize

        switch (position) {
            case 0: { // DOCK_TOP
                let visibleHeight = Math.max(0, dockGeometry.y + dockGeometry.height - screenGeometry.y)
                return Math.min(visibleHeight, dockSize)
            }
            case 1: { // DOCK_RIGHT
                let visibleWidth = Math.max(0, screenGeometry.x + screenGeometry.width - dockGeometry.x)
                return Math.min(visibleWidth, dockSize)
            }
            case 2: { // DOCK_BOTTOM
                let visibleHeight = Math.max(0, screenGeometry.y + screenGeometry.height - dockGeometry.y)
                return Math.min(visibleHeight, dockSize)
            }
            return 0
        }
    }

    visible: Applet.visible
    width: 390
    height: root.screen.height
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
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
            rightMargin: 10
            bottomMargin: 10
        }

        function updateInputRegion() {
            root.DLayerShellWindow.setInputRegionRect(
                Math.ceil(bubbleView.x),
                Math.ceil(bubbleView.y),
                Math.ceil(bubbleView.width), 
                Math.ceil(Math.max(10, bubbleView.contentHeight))
            )
        }
        onContentHeightChanged: updateInputRegion()
        onHeightChanged: updateInputRegion()
        onYChanged: updateInputRegion()

        spacing: 10
        model: Applet.bubbles
        interactive: false
        verticalLayoutDirection: ListView.BottomToTop
        add: Transition {
            id: addTrans
            PropertyAnimation {
                target: addTrans.ViewTransition.item
                properties: "x"
                from: addTrans.ViewTransition.item.width
                to: 0
                duration: 600
                easing.type: Easing.OutExpo
            }
        }

        addDisplaced: Transition {
            id: addDisplacedTrans
            PropertyAnimation {
                target: addDisplacedTrans.ViewTransition.item
                properties: "x"
                to: 0
                duration: 600
                easing.type: Easing.OutExpo
            }
            PropertyAnimation {
                target: addDisplacedTrans.ViewTransition.item
                properties: "y"
                duration: 600
                easing.type: Easing.OutExpo
            }
        }

        remove: Transition {
            id: removeTrans
            ParallelAnimation {
                PropertyAnimation {
                    target: removeTrans.ViewTransition.item
                    property: "opacity"
                    to: 0
                    duration: 400
                    easing.type: Easing.OutCubic
                }
            }
        }

        removeDisplaced: Transition {
            PropertyAnimation {
                properties: "opacity,y"
                duration: 400
                easing.type: Easing.OutExpo
            }
        }

        delegate: BubbleDelegate {
            maxCount: model.bubbleCount
        }

        HoverHandler {
            onPointChanged: {
                const local = point.position
                let hoveredItem = bubbleView.itemAt(local.x, local.y)
                if (hoveredItem && hoveredItem.bubble) {
                    Applet.setHoveredId(hoveredItem.bubble.id)
                } else {
                    Applet.setHoveredId(0)
                }
            }

            onHoveredChanged: {
                if (!hovered) {
                    Applet.setHoveredId(0)
                }
            }
        }
    }
}
