// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.ds.dock 1.0

Item {
    id: root

    required property int targetAlignment
    required property Item dockContent
    required property Item contentLayout
    required property Item iconContainer
    property bool animationAllowed: false
    // The layout may keep an intermediate alignment until the transition finishes.
    property int effectiveAlignment: targetAlignment
    property bool transitionActive: false
    property bool involvesCenteredMode: false
    property real iconMovementProgress: 0
    property real widthExpansionProgress: 0
    property bool componentInitialized: false
    property bool waitingForResize: false

    property real expandedDockWidth: 0
    property real fashionDockWidth: 0
    property real trailingAreaWidth: 0
    property real leftAlignedIconX: 0
    property real initialContentWidth: 0
    property real initialLayoutWidth: 0
    property real initialIconX: 0
    readonly property real compactDockWidth: targetAlignment === Dock.FashionAlignment
        ? fashionDockWidth : initialContentWidth
    readonly property real transitionLayoutWidth: targetAlignment === Dock.CenterAlignment ? expandedDockWidth
        : (targetAlignment === Dock.FashionAlignment
            ? Math.max(0, compactDockWidth - trailingAreaWidth) : initialLayoutWidth)
    readonly property int animatedDockWidth: Math.max(1, Math.round(compactDockWidth
        + (expandedDockWidth - compactDockWidth) * widthExpansionProgress))
    readonly property real iconTranslationX: {
        if (!transitionActive)
            return 0
        // Compensate for the moving left edge of the centered content.
        const contentOriginOffset = (dockContent.width - compactDockWidth) / 2
        const fashionIconX = targetAlignment === Dock.CenterAlignment ? initialIconX : iconContainer.x
        const centeredIconX = involvesCenteredMode && targetAlignment === Dock.FashionAlignment
            ? initialIconX : iconContainer.x
        const iconTravelDistance = (involvesCenteredMode ? centeredIconX : leftAlignedIconX)
            - fashionIconX - (expandedDockWidth - compactDockWidth) / 2
        // Centering uses the destination layout; other directions use the compact layout.
        const layoutOriginOffset = targetAlignment === Dock.CenterAlignment ? fashionIconX - centeredIconX : 0
        return contentOriginOffset + layoutOriginOffset
            + iconTravelDistance * (involvesCenteredMode ? widthExpansionProgress : iconMovementProgress)
    }
    readonly property bool contentWidthSettled: Math.abs(dockContent.width - animatedDockWidth) <= 1

    // Capture the current geometry before changing any visual bindings.
    signal transitionPreparing()

    function finishTransition() {
        modeSwitchAnimation.stop()
        effectiveAlignment = targetAlignment
        transitionActive = false
        waitingForResize = false
    }

    function finishAfterResize() {
        if (waitingForResize && contentWidthSettled)
            finishTransition()
    }

    onContentWidthSettledChanged: finishAfterResize()

    onTargetAlignmentChanged: {
        if (!componentInitialized)
            return

        const leavingFashionMode = effectiveAlignment === Dock.FashionAlignment
            && (targetAlignment === Dock.LeftAlignment || targetAlignment === Dock.CenterAlignment)
        const enteringFashionMode = (effectiveAlignment === Dock.LeftAlignment || effectiveAlignment === Dock.CenterAlignment)
            && targetAlignment === Dock.FashionAlignment
        if (!transitionActive && animationAllowed && (leavingFashionMode || enteringFashionMode)) {
            involvesCenteredMode = (enteringFashionMode ? effectiveAlignment : targetAlignment) === Dock.CenterAlignment
            transitionPreparing()
            initialContentWidth = dockContent.width
            initialLayoutWidth = contentLayout.width
            initialIconX = iconContainer.x
            iconMovementProgress = enteringFashionMode ? 1 : 0
            widthExpansionProgress = enteringFashionMode ? 1 : 0
            transitionActive = true
            // Use the normal destination layout for centered mode, and the
            // compact layout for transitions involving classic mode.
            effectiveAlignment = targetAlignment === Dock.CenterAlignment ? Dock.CenterAlignment : Dock.FashionAlignment
            modeSwitchAnimation.start()
        } else {
            finishTransition()
        }
    }

    onAnimationAllowedChanged: {
        if (!animationAllowed && transitionActive)
            finishTransition()
    }

    Component.onCompleted: {
        effectiveAlignment = targetAlignment
        componentInitialized = true
    }

    ParallelAnimation {
        id: modeSwitchAnimation
        NumberAnimation {
            target: root
            property: "widthExpansionProgress"
            to: root.targetAlignment === Dock.FashionAlignment ? 0 : 1
            duration: 600
            easing.type: Easing.InOutCubic
        }
        SequentialAnimation {
            // Classic icons follow expansion after 200 ms; contraction starts together.
            PauseAnimation {
                duration: root.targetAlignment === Dock.FashionAlignment || root.involvesCenteredMode ? 0 : 200
            }
            NumberAnimation {
                target: root
                property: "iconMovementProgress"
                to: root.targetAlignment === Dock.FashionAlignment || root.targetAlignment === Dock.CenterAlignment ? 0 : 1
                duration: root.involvesCenteredMode ? 0 : 400
                easing.type: Easing.InOutCubic
            }
        }
        // Keep coordinate compensation until the final Wayland configure lands.
        onFinished: {
            root.waitingForResize = true
            root.finishAfterResize()
        }
    }
}
