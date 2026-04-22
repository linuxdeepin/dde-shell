// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

PanelPopupWindow {
    id: root

    flags: Qt.ToolTip | Qt.WindowStaysOnTopHint
    D.DWindow.windowRadius: 8
    D.DWindow.shadowRadius: 8
    D.DWindow.shadowOffset: Qt.point(0, 8)
    property bool animatePosition: false
    property bool closingAnimationRequested: false
    property real contentScale: 1.0
    readonly property real hiddenContentScale: 0.96
    readonly property int motionDistanceLimit: 56
    readonly property int revealDuration: 132
    readonly property int dismissDuration: 108

    function resetVisualState() {
        root.opacity = 1.0
        root.contentScale = 1.0
        if (root.contentItem) {
            root.contentItem.opacity = 1.0
        }
    }

    function showAnimated() {
        hideAnimation.stop()
        closingAnimationRequested = false
        if (!root.visible) {
            root.opacity = 0.0
            root.contentScale = root.hiddenContentScale
            if (root.contentItem) {
                root.contentItem.opacity = 0.0
            }
            root.show()
            showAnimation.restart()
            return
        }

        resetVisualState()
    }

    function closeAnimated() {
        if (!root.visible) {
            root.currentItem = null
            return
        }

        if (hideAnimation.running) {
            return
        }

        showAnimation.stop()
        closingAnimationRequested = true
        hideAnimation.restart()
    }

    function syncAnimatedPosition() {
        const targetX = root.xOffset
        const targetY = root.yOffset
        const delta = Math.abs(targetX - root.positionXOffset) + Math.abs(targetY - root.positionYOffset)
        animatePosition = root.visible && !!root.currentItem && delta > 0 && delta <= motionDistanceLimit
        root.positionXOffset = targetX
        root.positionYOffset = targetY
    }

    onXOffsetChanged: syncAnimatedPosition()
    onYOffsetChanged: syncAnimatedPosition()
    onCurrentItemChanged: {
        if (!!root.currentItem && root.visible) {
            return
        }

        root.positionXOffset = root.xOffset
        root.positionYOffset = root.yOffset
        animatePosition = false
    }
    onVisibleChanged: {
        if (!visible) {
            animatePosition = false
            resetVisualState()
            return
        }

        root.positionXOffset = root.xOffset
        root.positionYOffset = root.yOffset
        if (!showAnimation.running && !hideAnimation.running) {
            resetVisualState()
        }
    }

    Binding {
        target: root.contentItem
        property: "transformOrigin"
        value: Item.Center
    }

    Binding {
        target: root.contentItem
        property: "scale"
        value: root.contentScale
    }

    ParallelAnimation {
        id: showAnimation

        NumberAnimation {
            target: root
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: root.revealDuration
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: root
            property: "contentScale"
            from: root.hiddenContentScale
            to: 1.0
            duration: root.revealDuration
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: root.contentItem
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: Math.round(root.revealDuration * 0.88)
            easing.type: Easing.OutQuad
        }
    }

    ParallelAnimation {
        id: hideAnimation

        NumberAnimation {
            target: root
            property: "opacity"
            from: root.opacity
            to: 0.0
            duration: root.dismissDuration
            easing.type: Easing.InCubic
        }

        NumberAnimation {
            target: root
            property: "contentScale"
            from: root.contentScale
            to: root.hiddenContentScale
            duration: root.dismissDuration
            easing.type: Easing.InCubic
        }

        NumberAnimation {
            target: root.contentItem
            property: "opacity"
            from: root.contentItem ? root.contentItem.opacity : 1.0
            to: 0.0
            duration: Math.round(root.dismissDuration * 0.92)
            easing.type: Easing.InQuad
        }

        onFinished: {
            if (!root.closingAnimationRequested) {
                return
            }

            root.closingAnimationRequested = false
            root.close()
            root.currentItem = null
        }
    }

    Behavior on positionXOffset {
        enabled: root.animatePosition
        NumberAnimation {
            duration: 72
            easing.type: Easing.OutQuad
        }
    }

    Behavior on positionYOffset {
        enabled: root.animatePosition
        NumberAnimation {
            duration: 72
            easing.type: Easing.OutQuad
        }
    }
}
