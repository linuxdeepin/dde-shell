// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 2.15
import QtQuick.Window 2.15

import QtQml
import Qt.labs.platform 1.1 as LP

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle

Window {
    id: dock
    readonly property int resizeScreenEdgeMargin: 10
    readonly property int adaptiveFashionLeftWidth: 160
    readonly property int adaptiveFashionMaximumWidth: Math.max(0, Screen.width - resizeScreenEdgeMargin * 2)
    property int positionForAnimation: Panel.position
    property bool useColumnLayout: positionForAnimation % 2
    readonly property bool adaptiveFashionMode: !useColumnLayout
        && Panel.viewMode === Dock.FashionMode
        && (positionForAnimation === Dock.Bottom || positionForAnimation === Dock.Top)
    readonly property bool adaptiveFashionTopMode: adaptiveFashionMode && positionForAnimation === Dock.Top
    readonly property bool adaptiveFashionBottomMode: adaptiveFashionMode && positionForAnimation === Dock.Bottom
    readonly property bool adaptiveFashionUsesDirectWindowGeometry: adaptiveFashionMode && Qt.platform.pluginName === "xcb"
    readonly property bool useWindowMarginBasedHideAnimation: adaptiveFashionMode
    readonly property bool useTransformBasedHideAnimation: Qt.platform.pluginName === "xcb" && !useWindowMarginBasedHideAnimation
    readonly property var promotedCenterPluginIds: ["org.deepin.ds.dock.aibar", "org.deepin.ds.dock.searchitem"]
    readonly property int fashionDockSpacing: Math.round(dockItemIconSize * 0.75)
    readonly property real fashionPartSpacing: adaptiveFashionMode
        ? dock.roundToPhysicalPixel(fashionDockSpacing)
        : fashionDockSpacing
    readonly property int fashionFloatingMargin: adaptiveFashionMode ? 8 : 0
    readonly property int fashionHorizontalPadding: 0
    readonly property int fashionVerticalPadding: adaptiveFashionMode ? Math.max(6, Math.round(dockSize * 0.16)) : 0
    readonly property int fashionBackgroundRadius: adaptiveFashionMode ? Math.round(dockSurfaceThickness / 4) : 0
    readonly property int fashionShadowRadius: adaptiveFashionMode ? Math.max(52, Math.round(dockSurfaceThickness * 0.8)) : 40
    readonly property int fashionShadowVerticalOffset: adaptiveFashionMode ? Math.max(6, Math.round(fashionBackgroundRadius * 0.35)) : 0
    readonly property bool useExternalFashionAutoHideSurface: adaptiveFashionMode && useTransformBasedHideAnimation
    property real animatedDockWindowMargin: adaptiveFashionMode ? fashionFloatingMargin : 0
    readonly property real panelDevicePixelRatio: Panel.devicePixelRatio > 0 ? Panel.devicePixelRatio : Screen.devicePixelRatio
    readonly property bool darkTheme: Panel.colorTheme === Dock.Dark
    readonly property var appearanceApplet: DS.applet("org.deepin.ds.dde-appearance")
    readonly property real appearanceOpacity: appearanceApplet ? appearanceApplet.opacity : -1
    readonly property real dockBackgroundOpacity: appearanceOpacity >= 0 ? appearanceOpacity : (darkTheme ? 85 / 255 : 0.6)
    readonly property color dockBlurBlendColor: darkTheme
        ? Qt.rgba(10 / 255, 10 / 255, 10 / 255, dockBackgroundOpacity)
        : Qt.rgba(235 / 255.0, 235 / 255.0, 235 / 255.0, dockBackgroundOpacity)
    readonly property color dockNoBlurBaseColor: darkTheme
        ? DStyle.Style.behindWindowBlur.darkNoBlurColor
        : DStyle.Style.behindWindowBlur.lightNoBlurColor
    readonly property color dockNoBlurBlendColor: Qt.rgba(dockNoBlurBaseColor.r,
        dockNoBlurBaseColor.g,
        dockNoBlurBaseColor.b,
        (appearanceOpacity >= 0 ? dockBackgroundOpacity : dockNoBlurBaseColor.a))
    readonly property color fashionShadowColor: darkTheme ?
        Qt.rgba(0, 0, 0, 0.34) :
        Qt.rgba(0, 0, 0, 0.2)
    readonly property color fashionInnerBorderTopColor: darkTheme
        ? Qt.rgba(1, 1, 1, 0.10)
        : Qt.rgba(1, 1, 1, 0.30)
    readonly property color fashionInnerBorderBottomColor: darkTheme
        ? Qt.rgba(1, 1, 1, 0.05)
        : Qt.rgba(1, 1, 1, 0.10)
    readonly property bool useTopRoundedFashionBackground: false
    readonly property int dockSurfaceThickness: useColumnLayout ? dockSize : (adaptiveFashionMode ? dockSize + fashionVerticalPadding * 2 : dockSize)
    readonly property int windowThickness: dockSize
    readonly property int exclusionZoneThickness: dock.windowThickness + (adaptiveFashionMode ? fashionFloatingMargin : 0)
    readonly property real adaptiveFashionGridDisplayedWidth: adaptiveFashionMode
        ? dock.ceilToPhysicalPixel(gridLayout.implicitWidth)
        : 0
    readonly property real adaptiveFashionGridDisplayedHeight: adaptiveFashionMode
        ? dock.ceilToPhysicalPixel(gridLayout.implicitHeight)
        : 0
    readonly property real adaptiveFashionLeftPartWidth: adaptiveFashionMode && dockLeftPart.visible
        ? dock.ceilToPhysicalPixel(dockLeftPart.implicitWidth)
        : 0
    readonly property real adaptiveFashionRightPartWidth: adaptiveFashionMode && dockRightPart.visible
        ? dock.ceilToPhysicalPixel(dockRightPart.targetImplicitWidth > 0 ? dockRightPart.targetImplicitWidth : dockRightPart.implicitWidth)
        : 0
    readonly property real adaptiveFashionAvailableCenterWidth: adaptiveFashionMode
        ? Math.max(0,
                   adaptiveFashionMaximumWidth
                   - adaptiveFashionLeftPartWidth
                   - adaptiveFashionRightPartWidth
                   - (adaptiveFashionLeftPartWidth > 0 ? fashionPartSpacing : 0)
                   - (adaptiveFashionRightPartWidth > 0 ? fashionPartSpacing : 0))
        : 0
    readonly property real adaptiveFashionAvailableTaskManagerWidth: adaptiveFashionMode
        ? Math.max(0,
                   adaptiveFashionAvailableCenterWidth
                   - Math.max(0, dockCenterPart.implicitWidth - dockCenterPart.taskmanagerAppContainerWidth))
        : 0
    readonly property real adaptiveDockContentWidth: {
        if (!adaptiveFashionMode) {
            return 0
        }

        let width = adaptiveFashionGridDisplayedWidth
        if (dockRightPart.visible) {
            if (width > 0) {
                width += fashionPartSpacing
            }
            width += dockRightPart.implicitWidth
        }
        return width
    }
    readonly property real adaptiveDockContentTargetWidth: {
        if (!adaptiveFashionMode) {
            return 0
        }

        let width = adaptiveFashionGridDisplayedWidth
        if (dockRightPart.visible) {
            const rightWidth = dockRightPart.targetImplicitWidth
            if (rightWidth > 0) {
                if (width > 0) {
                    width += fashionPartSpacing
                }
                width += rightWidth
            }
        }

        return width
    }
    readonly property int adaptiveFashionWidthAnimationDuration: 200
    readonly property int adaptiveFashionWidthAnimationEasingType: DDT.TraySortOrderModel.collapsed || !DDT.TraySortOrderModel.isCollapsing
        ? Easing.OutQuad
        : Easing.InQuad
    readonly property bool adaptiveFashionWidthAnimationEnabled: adaptiveFashionMode
        && dock.adaptiveFashionUsesDirectWindowGeometry
        && !dock.isDragging
        && !Panel.isResizing
        && !DDT.TraySortOrderModel.actionsAlwaysVisible
    property real adaptiveDockShellWidth: 0
    // TODO: 临时溢出逻辑，待后面修改
    property int dockLeftSpaceForCenter: adaptiveFashionMode ? 0 : (useColumnLayout ?
        (Screen.height - dockLeftPart.implicitHeight - dockRightPart.implicitHeight) :
        (Screen.width - dockLeftPart.implicitWidth - dockRightPart.implicitWidth))
    property int dockRemainingSpaceForCenter: adaptiveFashionMode ? 0 : (useColumnLayout ?
        (Screen.height / 1.8 - dockRightPart.implicitHeight)  :
        (Screen.width / 1.8 - dockRightPart.implicitWidth))
    property real dockPartSpacing: adaptiveFashionMode ? fashionPartSpacing : gridLayout.columnSpacing
    // TODO
    signal dockCenterPartPosChanged()
    signal pressedAndDragging(bool isDragging)
    signal viewDeactivated()

    property int dockCenterPartCount: dockCenterPartModel.count

    property int preferredDockSize: Applet.dockSize
    property int dockSize: Applet.dockSize
    property int dockItemMaxSize: dockSize
    property int itemIconSizeBase: 0
    property int itemSpacing: 0

    property bool isDragging: false
    property real spotlightX: 0
    property real spotlightY: 0
    readonly property int spotlightHideGraceInterval: 180
    readonly property bool spotlightActive: Panel.containsMouse || spotlightTracker.hovered || spotlightHideGraceTimer.running

    property real dockItemIconSize: dockItemMaxSize * 9 / 14
    readonly property real spotlightBaseRadius: Math.max(264, dockItemMaxSize * (adaptiveFashionMode ? 8.0 : 6.8))
    readonly property real spotlightCoreRadius: Math.max(96, dockItemMaxSize * (adaptiveFashionMode ? 2.8 : 2.4))
    readonly property real spotlightHorizontalRadius: useColumnLayout
        ? Math.max(dockSurfaceThickness * 2.24, spotlightBaseRadius * 1.56)
        : spotlightBaseRadius
    readonly property real spotlightVerticalRadius: useColumnLayout
        ? spotlightBaseRadius
        : Math.max(dockSurfaceThickness * 2.12, spotlightBaseRadius * 1.3)
    readonly property real spotlightOpacity: darkTheme ? 0.14 : 0.2
    readonly property real spotlightCoreOpacity: darkTheme ? 0.23 : 0.32
    readonly property color dockWindowBorderColor: darkTheme ?
        Qt.rgba(0, 0, 0, dock.dockBackgroundOpacity + 20 / 255) :
        Qt.rgba(0, 0, 0, 0.15)

    // NOTE: -1 means not set its size, follow the platform size
    width: {
        if (dock.adaptiveFashionUsesDirectWindowGeometry) {
            return Math.max(1, dock.adaptiveDockShellWidth + dock.fashionHorizontalPadding * 2)
        }

        return positionForAnimation === Dock.Top || positionForAnimation === Dock.Bottom ? -1 : dockSize
    }
    height: positionForAnimation === Dock.Left || positionForAnimation === Dock.Right ? -1 : windowThickness
    color: "transparent"
    flags: Qt.WindowDoesNotAcceptFocus

    function requestShowDockMenu() {
        // maybe has popup visible, close it.
        Panel.requestClosePopup()
        viewDeactivated()
        hideTimer.stop()
        MenuHelper.openMenu(dockMenuLoader.item)
    }

    function handleDockBackgroundClick(button) {
        const lastActive = MenuHelper.activeMenu
        MenuHelper.closeCurrent()
        dockMenuLoader.active = true

        if (button === Qt.RightButton) {
            if (lastActive !== dockMenuLoader.item) {
                requestShowDockMenu()
            }
            return
        }

        if (button === Qt.LeftButton) {
            // try to close popup when clicked empty, because dock does not have focus.
            Panel.requestClosePopup()
            viewDeactivated()
        }
    }

    function containsItemPoint(item, x, y) {
        if (!item || !item.visible || item.width <= 0 || item.height <= 0) {
            return false
        }

        const mapped = item.mapFromItem(dockContainer, x, y)
        return mapped.x >= 0 && mapped.x < item.width && mapped.y >= 0 && mapped.y < item.height
    }

    function isDockBlankAreaPoint(x, y) {
        if (dock.adaptiveFashionMode) {
            return false
        }

        if (dock.containsItemPoint(dockTrailingBlankArea, x, y)) {
            return true
        }

        return Panel.viewMode === Dock.LeftAlignedMode
            && dock.containsItemPoint(leftMarginArea, x, y)
    }

    function hiddenTransformOffset() {
        return (dock.positionForAnimation === Dock.Left || dock.positionForAnimation === Dock.Top)
            ? -dock.windowThickness
            : dock.windowThickness
    }

    function syncDockHiddenGeometryForCurrentMode() {
        dock.positionForAnimation = Panel.position
        hideShowAnimation.stop()
        dockAnimation.stop()
        changeDragAreaAnchor()

        if (dock.useWindowMarginBasedHideAnimation) {
            dockTransform.x = 0
            dockTransform.y = 0
            dock.animatedDockWindowMargin = dock.restingWindowMargin()
            dock.visible = false
            Panel.notifyDockPositionChanged(0, 0)
            return
        }

        if (dock.useTransformBasedHideAnimation) {
            if (dock.useColumnLayout) {
                dockTransform.x = dock.hiddenTransformOffset()
                dockTransform.y = 0
            } else {
                dockTransform.x = 0
                dockTransform.y = dock.hiddenTransformOffset()
            }

            dock.visible = false
            Panel.notifyDockPositionChanged(dockTransform.x, dockTransform.y)
            return
        }

        dockTransform.x = 0
        dockTransform.y = 0
        dock.animatedDockWindowMargin = dock.restingWindowMargin()
        dock.visible = false
        Panel.notifyDockPositionChanged(0, 0)
    }

    function syncDockShownGeometryForCurrentMode() {
        dock.positionForAnimation = Panel.position
        hideShowAnimation.stop()
        dockAnimation.stop()
        changeDragAreaAnchor()

        dockTransform.x = 0
        dockTransform.y = 0
        dock.animatedDockWindowMargin = dock.restingWindowMargin()
        dock.visible = true
        Panel.notifyDockPositionChanged(0, 0)
        shownContentSyncTimer.restart()
    }

    function syncDockPresentationForCurrentMode() {
        dock.scheduleAdaptiveFashionDockSizeSync()

        if (Panel.hideState === Dock.Hide) {
            dock.syncDockHiddenGeometryForCurrentMode()
        } else {
            dock.syncDockShownGeometryForCurrentMode()
        }

        dock.scheduleFrontendGeometrySync()
    }

    function clampSpotlightPosition(value, maximum) {
        return Math.max(0, Math.min(maximum, value))
    }

    function hiddenWindowMargin() {
        return dock.fashionFloatingMargin
            - dock.windowThickness
            - dock.fashionShadowRadius
            - dock.fashionShadowVerticalOffset
            - 4
    }

    function restingWindowMargin() {
        if (!dock.adaptiveFashionMode) {
            return 0
        }

        return Panel.hideState !== Dock.Hide ? dock.fashionFloatingMargin : dock.hiddenWindowMargin()
    }

    function adaptiveHorizontalMarginForContentWidth(contentWidth) {
        if (!adaptiveFashionMode) {
            return 0
        }

        const desiredWidth = Math.max(0, contentWidth) + fashionHorizontalPadding * 2
        return Math.max(0, Math.floor((adaptiveFashionMaximumWidth - desiredWidth) / 2))
    }

    function roundToPhysicalPixel(value) {
        if (!Number.isFinite(value) || dock.panelDevicePixelRatio <= 0) {
            return value
        }

        return Math.round(value * dock.panelDevicePixelRatio) / dock.panelDevicePixelRatio
    }

    function ceilToPhysicalPixel(value) {
        if (!Number.isFinite(value) || dock.panelDevicePixelRatio <= 0) {
            return value
        }

        return Math.ceil(value * dock.panelDevicePixelRatio) / dock.panelDevicePixelRatio
    }

    function appletPluginId(item) {
        return item && item.applet ? item.applet.pluginId : ""
    }

    function dockOrderInRange(item, leftDockOrder, rightDockOrder) {
        const order = Number(item ? item.dockOrder : NaN)
        return Number.isFinite(order) && order > leftDockOrder && order <= rightDockOrder
    }

    function dockAppletVisible(item) {
        return item && (item.shouldVisible === undefined || item.shouldVisible)
    }

    function isPromotedCenterApplet(item) {
        return adaptiveFashionMode && promotedCenterPluginIds.indexOf(appletPluginId(item)) >= 0
    }

    function acceptAdaptiveLeftDockApplet(item) {
        const appletItem = item.data
        return dockAppletVisible(appletItem) && dockOrderInRange(appletItem, 0, 10) && !isPromotedCenterApplet(appletItem)
    }

    function acceptAdaptiveCenterDockApplet(item) {
        const appletItem = item.data
        return dockAppletVisible(appletItem) && (dockOrderInRange(appletItem, 10, 20) || isPromotedCenterApplet(appletItem))
    }

    function acceptAdaptiveRightDockApplet(item) {
        const appletItem = item.data
        return dockAppletVisible(appletItem)
            && dockOrderInRange(appletItem, 20, 30)
            && appletPluginId(appletItem) !== "org.deepin.ds.dock.showdesktop"
    }

    function adaptiveCenterSortOrder(item) {
        const appletItem = item ? item.data : null
        const pluginId = appletPluginId(appletItem)
        switch (pluginId) {
        case "org.deepin.ds.dock.launcherapplet":
            return 1200
        case "org.deepin.ds.dock.aibar":
            return 1300
        case "org.deepin.ds.dock.searchitem":
            return 1400
        default: {
            const order = Number(appletItem ? appletItem.dockOrder : NaN)
            return Number.isFinite(order) ? order * 100 : 0
        }
        }
    }

    function clampDockSizeByScreenLimit(proposedDockSize) {
        if (useColumnLayout || !adaptiveFashionMode || dockSize <= 0) {
            return proposedDockSize
        }

        const maximumWidth = adaptiveFashionMaximumWidth
        if (maximumWidth <= 0) {
            return Dock.MIN_DOCK_SIZE
        }

        const currentWidth = adaptiveDockContentTargetWidth + fashionHorizontalPadding * 2
        if (currentWidth <= 0) {
            return proposedDockSize
        }

        const estimatedWidth = currentWidth * proposedDockSize / dockSize
        if (estimatedWidth <= maximumWidth) {
            return proposedDockSize
        }

        return Math.max(Dock.MIN_DOCK_SIZE,
                        Math.min(Dock.MAX_DOCK_SIZE,
                                 Math.floor(proposedDockSize * maximumWidth / estimatedWidth)))
    }

    function syncAdaptiveFashionDockSize() {
        if (!adaptiveFashionMode || useColumnLayout) {
            if (dock.dockSize !== dock.preferredDockSize) {
                dock.dockSize = dock.preferredDockSize
            }
            return
        }

        if (dock.preferredDockSize <= 0) {
            return
        }

        const fittedDockSize = dock.clampDockSizeByScreenLimit(dock.preferredDockSize)
        if (dock.dockSize !== fittedDockSize) {
            dock.dockSize = fittedDockSize
        }
    }

    function effectiveAdaptiveFashionShellWidth() {
        if (!adaptiveFashionMode) {
            return 0
        }

        if (adaptiveDockContentTargetWidth > 0) {
            return dock.ceilToPhysicalPixel(adaptiveDockContentTargetWidth)
        }

        return dock.ceilToPhysicalPixel(adaptiveDockContentWidth)
    }

    function scheduleAdaptiveFashionDockSizeSync() {
        if (dock.useColumnLayout) {
            return
        }

        if (dock._adaptiveFashionDockSizeSyncPending) {
            return
        }

        dock._adaptiveFashionDockSizeSyncPending = true
        Qt.callLater(function() {
            dock._adaptiveFashionDockSizeSyncPending = false
            dock.syncAdaptiveFashionDockSize()
        })
    }

    function scheduleFrontendGeometrySync() {
        if (dock._frontendGeometrySyncPending) {
            return
        }

        dock._frontendGeometrySyncPending = true
        Qt.callLater(function() {
            dock._frontendGeometrySyncPending = false
            Panel.notifyDockPositionChanged(0, 0)
        })
    }

    function refreshAdaptiveFashionGeometry() {
        dock.syncDockPresentationForCurrentMode()
    }

    function setDockViewMode(mode) {
        Applet.viewMode = mode
    }

    Path {
        id: topRoundedFashionWindowClipPath
        startX: 0
        startY: dock.height

        PathLine {
            x: 0
            y: dock.fashionBackgroundRadius
        }
        PathArc {
            x: dock.fashionBackgroundRadius
            y: 0
            radiusX: dock.fashionBackgroundRadius
            radiusY: dock.fashionBackgroundRadius
            direction: PathArc.Clockwise
        }
        PathLine {
            x: dock.width - dock.fashionBackgroundRadius
            y: 0
        }
        PathArc {
            x: dock.width
            y: dock.fashionBackgroundRadius
            radiusX: dock.fashionBackgroundRadius
            radiusY: dock.fashionBackgroundRadius
            direction: PathArc.Clockwise
        }
        PathLine {
            x: dock.width
            y: dock.height
        }
        PathLine {
            x: 0
            y: dock.height
        }
    }

    DLayerShellWindow.anchors: dock.adaptiveFashionUsesDirectWindowGeometry
        ? (dock.adaptiveFashionTopMode ? DLayerShellWindow.AnchorTop : DLayerShellWindow.AnchorBottom)
        : position2Anchors(positionForAnimation)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.exclusionZone: Panel.hideMode === Dock.KeepShowing ? dock.exclusionZoneThickness : 0
    DLayerShellWindow.leftMargin: dock.adaptiveFashionUsesDirectWindowGeometry
        ? 0
        : (adaptiveFashionMode ? dock.adaptiveHorizontalMarginForContentWidth(dock.adaptiveDockShellWidth) : 0)
    DLayerShellWindow.rightMargin: dock.adaptiveFashionUsesDirectWindowGeometry
        ? 0
        : (adaptiveFashionMode ? dock.adaptiveHorizontalMarginForContentWidth(dock.adaptiveDockShellWidth) : 0)
    DLayerShellWindow.topMargin: dock.adaptiveFashionTopMode
        ? (dock.useWindowMarginBasedHideAnimation
            ? dock.animatedDockWindowMargin
            : dock.restingWindowMargin())
        : 0
    DLayerShellWindow.bottomMargin: dock.adaptiveFashionBottomMode
        ? (dock.useWindowMarginBasedHideAnimation
            ? dock.animatedDockWindowMargin
            : dock.restingWindowMargin())
        : 0
    DLayerShellWindow.scope: "dde-shell/dock"
    DLayerShellWindow.keyboardInteractivity: DLayerShellWindow.KeyboardInteractivityOnDemand

    D.DWindow.enabled: true
    D.DWindow.windowRadius: useTopRoundedFashionBackground ? 0 : fashionBackgroundRadius
    D.DWindow.clipPath: useTopRoundedFashionBackground ? topRoundedFashionWindowClipPath : null
    //TODO：由于windoweffect处理有BUG，导致动画结束后一致保持无阴影，无borderwidth状态。 无法恢复到最初的阴影和边框
    //D.DWindow.windowEffect: hideShowAnimation.running ? D.PlatformHandle.EffectNoShadow | D.PlatformHandle.EffectNoBorder : 0
    
    // 目前直接处理shadowColor(透明和默认值的切换)和borderWidth(0和1的切换)，来控制阴影和边框
    // 参数默认值见： https://github.com/linuxdeepin/qt5platform-plugins/blob/master/xcb/dframewindow.h#L122
    // 需要注意，shadowRadius不能直接套用于“扩散”参数，拿到不透明度100%的设计图确定radius更合适一些。
    D.DWindow.shadowColor: {
        if (dockAnimation.running) {
            return Qt.rgba(0, 0, 0, 0)
        }

        if (dock.adaptiveFashionMode) {
            return dock.fashionShadowColor
        }

        if (hideShowAnimation.running) {
            return Qt.rgba(0, 0, 0, 0)
        }

        return Qt.rgba(0, 0, 0, 0.1)
    }
    D.DWindow.shadowOffset: dock.adaptiveFashionMode ? Qt.point(0, dock.fashionShadowVerticalOffset) : Qt.point(0, 0)
    D.DWindow.shadowRadius: dock.adaptiveFashionMode ? dock.fashionShadowRadius : 40
    D.DWindow.borderWidth: dock.adaptiveFashionMode ? 1 : ((hideShowAnimation.running || dockAnimation.running) ? 0 : 1)
    D.DWindow.enableBlurWindow: Qt.platform.pluginName !== "xcb"
    D.DWindow.themeType: Panel.colorTheme
    D.DWindow.borderColor: dock.dockWindowBorderColor
    D.ColorSelector.family: D.Palette.CrystalColor

    onDockSizeChanged: {
        if (dock.adaptiveFashionMode) {
            Panel.indicatorStyle = Dock.Fashion
        } else if (dock.dockSize === Dock.MIN_DOCK_SIZE) {
            Panel.indicatorStyle = Dock.Efficient
        } else {
            Panel.indicatorStyle = Dock.Fashion
        }
    }

    onAdaptiveFashionModeChanged: {
        updateAppItems()
        dock.refreshAdaptiveFashionGeometry()
    }

    onAdaptiveDockContentWidthChanged: {
        if (dock.adaptiveFashionMode) {
            Panel.notifyDockPositionChanged(0, 0)
        }
    }
    onAdaptiveDockContentTargetWidthChanged: {
        dock.scheduleAdaptiveFashionDockSizeSync()
    }
    onAdaptiveFashionMaximumWidthChanged: dock.scheduleAdaptiveFashionDockSizeSync()
    onWidthChanged: {
        if (dock.adaptiveFashionMode) {
            dock.scheduleAdaptiveFashionDockSizeSync()
        }
    }

    property bool _adaptiveFashionDockSizeSyncPending: false
    property bool _frontendGeometrySyncPending: false

    Binding {
        target: dock
        property: "adaptiveDockShellWidth"
        value: dock.effectiveAdaptiveFashionShellWidth()
        restoreMode: Binding.RestoreNone
    }

    Behavior on adaptiveDockShellWidth {
        enabled: dock.adaptiveFashionWidthAnimationEnabled
        NumberAnimation {
            duration: dock.adaptiveFashionWidthAnimationDuration
            easing.type: dock.adaptiveFashionWidthAnimationEasingType
        }
    }

    Binding on itemIconSizeBase {
        when: !isDragging
        value: dockItemMaxSize
        restoreMode: Binding.RestoreNone
    }

    QtObject {
        id: hideShowAnimation;
        // Currently, Wayland (Treeland) doesn't support StyledBehindWindowBlur inside the window, thus we keep using the window size approach on Wayland
        readonly property bool useWindowMarginBasedAnimation: dock.useWindowMarginBasedHideAnimation
        readonly property bool useTransformBasedAnimation: dock.useTransformBasedHideAnimation
        readonly property int showDuration: useWindowMarginBasedAnimation ? 120 : 160
        readonly property int hideDuration: useWindowMarginBasedAnimation ? 180 : 220
        readonly property bool running: hideShowMarginAnimation.running
            || hideShowTransformAnimation.running
            || hideShowSizeAnimation.running

        function stop() {
            hideShowMarginAnimation.stop()
            hideShowTransformAnimation.stop()
            hideShowSizeAnimation.stop()
        }

        function finalize() {
            dock.visible = Panel.hideState !== Dock.Hide
            if (dock.visible) {
                shownContentSyncTimer.restart()
            }
        }

        function start() {
            dock.visible = true

            if (useWindowMarginBasedAnimation) {
                hideShowMarginAnimation.stop()
                hideShowMarginAnimation.from = dock.animatedDockWindowMargin
                hideShowMarginAnimation.to = dock.restingWindowMargin()
                hideShowMarginAnimation.duration = Panel.hideState !== Dock.Hide ? showDuration : hideDuration
                hideShowMarginAnimation.start()
                return
            }

            if (useTransformBasedAnimation) {
                const transformProperty = dock.useColumnLayout ? "x" : "y"
                hideShowTransformAnimation.stop()
                hideShowTransformAnimation.property = transformProperty
                hideShowTransformAnimation.from = dock.useColumnLayout ? dockTransform.x : dockTransform.y
                hideShowTransformAnimation.to = Panel.hideState !== Dock.Hide ? 0 : dock.hiddenTransformOffset()
                hideShowTransformAnimation.duration = Panel.hideState !== Dock.Hide ? showDuration : hideDuration
                hideShowTransformAnimation.start()
                return
            }

            const sizeProperty = dock.useColumnLayout ? "width" : "height"
            hideShowSizeAnimation.stop()
            hideShowSizeAnimation.property = sizeProperty
            hideShowSizeAnimation.from = dock.useColumnLayout ? dock.width : dock.height
            hideShowSizeAnimation.to = Panel.hideState !== Dock.Hide ? dock.windowThickness : 1
            hideShowSizeAnimation.duration = Panel.hideState !== Dock.Hide ? showDuration : hideDuration
            hideShowSizeAnimation.start()
        }

        function restart() {
            stop()
            start()
        }
    }

    PropertyAnimation {
        id: hideShowMarginAnimation
        target: dock
        property: "animatedDockWindowMargin"
        easing.type: Easing.OutCubic
        onStopped: hideShowAnimation.finalize()
    }

    PropertyAnimation {
        id: hideShowTransformAnimation
        target: dockTransform
        property: "x"
        easing.type: Easing.OutCubic
        onStopped: hideShowAnimation.finalize()
    }

    PropertyAnimation {
        id: hideShowSizeAnimation
        target: dock
        property: "height"
        easing.type: Easing.OutCubic
        onStopped: hideShowAnimation.finalize()
    }

    Connections {
        target: dockTransform
        enabled: hideShowAnimation.useTransformBasedAnimation && hideShowAnimation.running
        
        function onXChanged() {
            if (dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(dockTransform.x, 0)
            }
        }
        
        function onYChanged() {
            if (!dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(0, dockTransform.y)
            }
        }
    }

    Connections {
        target: dock
        enabled: hideShowAnimation.useWindowMarginBasedAnimation && hideShowAnimation.running

        function onAnimatedBottomMarginChanged() {
            Panel.notifyDockPositionChanged(0, 0)
        }
    }

    Connections {
        target: dock
        enabled: !hideShowAnimation.useTransformBasedAnimation
            && !hideShowAnimation.useWindowMarginBasedAnimation
            && hideShowAnimation.running
        
        function onWidthChanged() {
            if (dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(dock.width, 0)
            }
        }
        
        function onHeightChanged() {
            if (!dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(0, dock.height)
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 1
        running: false
        repeat: false
        onTriggered: {
            if (!dock.isDragging && !MenuHelper.activeMenu)
                hideShowAnimation.start()
        }
    }

    Timer {
        id: shownContentSyncTimer
        interval: 0
        running: false
        repeat: false
        onTriggered: {
            if (!dock.visible || Panel.hideState === Dock.Hide) {
                return
            }

            updateAppItems()
            dock.scheduleAdaptiveFashionDockSizeSync()
            dock.scheduleFrontendGeometrySync()
        }
    }

    SequentialAnimation {
        id: dockAnimation
        property bool useTransformBasedAnimation: Qt.platform.pluginName === "xcb"
        property bool isShowing: false
        property bool isPositionChanging: false
        property var target: useTransformBasedAnimation ? dockTransform : dock
        property string animProperty: {
            if (useTransformBasedAnimation) return dock.useColumnLayout ? "x" : "y";
            return dock.useColumnLayout ? "width" : "height";
        }

        function startAnimation(showing) {
            isShowing = showing;
            start();
        }

        function setTransformToHiddenPosition() {
            if (useTransformBasedAnimation) {
                var hideOffset = (Panel.position === Dock.Left || Panel.position === Dock.Top) ? -dock.windowThickness : dock.windowThickness;
                if (dock.useColumnLayout) {
                    dockTransform.x = hideOffset;
                    dockTransform.y = 0;
                } else {
                    dockTransform.x = 0;
                    dockTransform.y = hideOffset;
                }
            } else {
                dockTransform.x = 0;
                dockTransform.y = 0;
            }
        }

        PropertyAnimation {
            target: dockAnimation.target
            property: dockAnimation.animProperty
            from: {
                if (dockAnimation.isShowing) {
                    if (dockAnimation.useTransformBasedAnimation) {
                        return (dock.positionForAnimation === Dock.Left || dock.positionForAnimation === Dock.Top) ? -dock.windowThickness : dock.windowThickness;
                    }
                    return 1;
                }
                return dockAnimation.useTransformBasedAnimation ? 0 : dock.windowThickness;
            }
            to: {
                if (dockAnimation.isShowing) {
                    return dockAnimation.useTransformBasedAnimation ? 0 : dock.windowThickness;
                } else {
                    if (dockAnimation.useTransformBasedAnimation) {
                        return (dock.positionForAnimation === Dock.Left || dock.positionForAnimation === Dock.Top) ? -dock.windowThickness : dock.windowThickness;
                    }
                    return 1;
                }
            }
            duration: 250
            easing.type: Easing.OutCubic
        }

        onStarted: {
            dock.visible = true;
        }

        onStopped: {
            if (useTransformBasedAnimation) {
                dock.visible = true;
            } else {
                dock.visible = ((dock.useColumnLayout ? dock.width : dock.height) !== 1);
            }

            if (dock.visible) {
                shownContentSyncTimer.restart()
            }

            dock.positionForAnimation = Panel.position;
            changeDragAreaAnchor()
            // If this was a hide animation during position change, prepare for show animation
            if (isPositionChanging && !isShowing) {
                isPositionChanging = false;

                // Set transform to hidden position before showing
                setTransformToHiddenPosition();

                startAnimation(true);
            } else if (isShowing) {
                // After show animation completes, check if we need to auto-hide
                // For KeepHidden and SmartHide modes, trigger hide check immediately
                if (Panel.hideMode === Dock.KeepHidden || Panel.hideMode === Dock.SmartHide) {
                    hideTimer.running = true;
                } else if (Panel.hideState === Dock.Hide) {
                    // For other cases, if hideState is already Hide, trigger hide animation
                    hideTimer.running = true;
                }
            }
        }
    }

    component EnumPropertyMenuItem: LP.MenuItem {
        required property string name
        required property string prop
        required property int value
        text: name

        onTriggered: {
            Applet[prop] = value
            checked = Qt.binding(function() {
                return Applet[prop] === value
            })
        }
        checked: Applet[prop] === value
    }
    component MutuallyExclusiveMenu: LP.Menu {
        id: menu
        LP.MenuItemGroup {
            id: group
            items: menu.items
        }
    }

    function updateAppItems()
    {
        if (!dockLeftPartModel || !dockCenterPartModel || !dockRightPartModel) {
            return
        }

        dockLeftPartModel.update()
        dockCenterPartModel.update()
        dockRightPartModel.update()
    }

    Loader {
        id: dockMenuLoader
        active: false
        sourceComponent: LP.Menu {
            id: dockMenu
            MutuallyExclusiveMenu {
                visible: Panel.debugMode
                title: qsTr("Indicator Style")
                EnumPropertyMenuItem {
                    name: qsTr("Fashion Mode")
                    prop: "indicatorStyle"
                    value: Dock.Fashion
                }
                EnumPropertyMenuItem {
                    name: qsTr("Efficient Mode")
                    prop: "indicatorStyle"
                    value: Dock.Efficient
                }
            }
            MutuallyExclusiveMenu {
                title: qsTr("Mode")
                EnumPropertyMenuItem {
                    name: qsTr("Left-aligned Mode")
                    prop: "viewMode"
                    value: Dock.LeftAlignedMode
                }
                EnumPropertyMenuItem {
                    name: qsTr("Centered Mode")
                    prop: "viewMode"
                    value: Dock.CenteredMode
                }
                EnumPropertyMenuItem {
                    name: qsTr("Fashion Mode")
                    prop: "viewMode"
                    value: Dock.FashionMode
                }
            }
            MutuallyExclusiveMenu {
                title: qsTr("Position")
                EnumPropertyMenuItem {
                    name: qsTr("Top")
                    prop: "position"
                    value: Dock.Top
                }
                EnumPropertyMenuItem {
                    name: qsTr("Bottom")
                    prop: "position"
                    value: Dock.Bottom
                }
                EnumPropertyMenuItem {
                    name: qsTr("Left")
                    prop: "position"
                    value: Dock.Left
                }
                EnumPropertyMenuItem {
                    name: qsTr("Right")
                    prop: "position"
                    value: Dock.Right
                }
            }
            MutuallyExclusiveMenu {
                title: qsTr("Status")
                EnumPropertyMenuItem {
                    name: qsTr("Keep Shown")
                    prop: "hideMode"
                    value: Dock.KeepShowing
                }
                EnumPropertyMenuItem {
                    name: qsTr("Keep Hidden")
                    prop: "hideMode"
                    value: Dock.KeepHidden
                }
                EnumPropertyMenuItem {
                    name: qsTr("Smart Hide")
                    prop: "hideMode"
                    value: Dock.SmartHide
                }
            }
            LP.MenuItem {
                text: qsTr("Lock the Dock")
                checked: Panel.locked
                onTriggered: {
                    Panel.locked = !Panel.locked
                }
            }
            LP.MenuItem {
                text: qsTr("Dock Settings")
                onTriggered: {
                    Panel.openDockSettings()
                }
            }
        }
    }

    Item {
        id: fashionAutoHideSurfaceLayer
        visible: dock.useExternalFashionAutoHideSurface
        width: dock.useColumnLayout ? dock.dockSize : dock.width
        height: dock.useColumnLayout ? dock.height : dock.windowThickness
        x: dock.useColumnLayout ? dockTransform.x : 0
        y: dock.useColumnLayout ? 0 : dockTransform.y
        z: 0

        D.StyledBehindWindowBlur {
            control: fashionAutoHideSurfaceLayer
            anchors.fill: parent
            cornerRadius: dock.fashionBackgroundRadius
            blendColor: {
                if (valid) {
                    return dock.dockBlurBlendColor
                }
                return dock.dockNoBlurBlendColor
            }
        }

        FashionBackgroundInnerBorder {
            anchors.fill: parent
            visible: dock.adaptiveFashionMode
            cornerRadius: dock.fashionBackgroundRadius
            devicePixelRatio: dock.panelDevicePixelRatio
            topColor: dock.fashionInnerBorderTopColor
            bottomColor: dock.fashionInnerBorderBottomColor
        }
    }

    Item {
        id: dockContainer
        z: 1
        width: dock.useColumnLayout ? dock.dockSize : parent.width
        height: dock.useColumnLayout ? parent.height : dock.windowThickness
        anchors {
            left: parent.left
            top: parent.top
        }
        transform: Translate {
            id: dockTransform
        }

        // only add blendColor effect when DWindow.enableBlurWindow is true,
        // avoid to updating blur area frequently.--
        D.StyledBehindWindowBlur {
            id: dockBackgroundBlur
            control: parent
            anchors.fill: parent
            visible: !dock.useTopRoundedFashionBackground && !dock.useExternalFashionAutoHideSurface
            cornerRadius: dock.adaptiveFashionMode ? dock.fashionBackgroundRadius : 0
            blendColor: {
                if (valid) {
                    return dock.dockBlurBlendColor
                }
                return dock.dockNoBlurBlendColor
            }
        }

        FashionBackgroundInnerBorder {
            anchors.fill: parent
            visible: dock.adaptiveFashionMode
                && !dock.useTopRoundedFashionBackground
                && !dock.useExternalFashionAutoHideSurface
            cornerRadius: dock.fashionBackgroundRadius
            devicePixelRatio: dock.panelDevicePixelRatio
            topColor: dock.fashionInnerBorderTopColor
            bottomColor: dock.fashionInnerBorderBottomColor
        }

        Item {
            id: topRoundedFashionBackground
            anchors.fill: parent
            visible: dock.useTopRoundedFashionBackground && !dock.useExternalFashionAutoHideSurface

            Item {
                id: topRoundedFashionSurface
                anchors.fill: parent

                D.StyledBehindWindowBlur {
                    control: topRoundedFashionBackground
                    anchors.fill: parent
                    cornerRadius: 0
                    blendColor: {
                        if (valid) {
                            return dock.dockBlurBlendColor
                        }
                        return dock.dockNoBlurBlendColor
                    }
                }

            }
        }

        Item {
            id: spotlightLayer
            anchors.fill: parent
            clip: true
            visible: dock.spotlightActive || opacity > 0.01
            opacity: dock.spotlightActive ? 1 : 0
            enabled: false

            Behavior on opacity {
                NumberAnimation {
                    duration: 90
                    easing.type: Easing.OutQuad
                }
            }

            Image {
                id: spotlightImage
                width: dock.spotlightHorizontalRadius * 2
                height: dock.spotlightVerticalRadius * 2
                x: dock.clampSpotlightPosition(dock.spotlightX, spotlightLayer.width) - width / 2
                y: dock.clampSpotlightPosition(dock.spotlightY, spotlightLayer.height) - height / 2
                source: "SpotlightGradient.png"
                fillMode: Image.Stretch
                smooth: true
                cache: true
                asynchronous: false
                opacity: dock.spotlightOpacity
            }

            Image {
                id: spotlightCoreImage
                width: dock.spotlightCoreRadius * 2
                height: dock.spotlightCoreRadius * 2
                x: dock.clampSpotlightPosition(dock.spotlightX, spotlightLayer.width) - width / 2
                y: dock.clampSpotlightPosition(dock.spotlightY, spotlightLayer.height) - height / 2
                source: "SpotlightGradient.png"
                fillMode: Image.Stretch
                smooth: true
                cache: true
                asynchronous: false
                opacity: dock.spotlightCoreOpacity
            }
        }

        MouseArea {
            id: dockBackgroundMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouse) {
                dock.handleDockBackgroundClick(mouse.button)
            }
        }

        //Touch screen click
        TapHandler {
            acceptedButtons: Qt.NoButton
            acceptedDevices: PointerDevice.TouchScreen
            onTapped: function(eventPoint, button) {
                let lastActive = MenuHelper.activeMenu
                MenuHelper.closeCurrent()
                dockMenuLoader.active = true
                // try to close popup when clicked empty, because dock does not have focus.
                Panel.requestClosePopup()
                viewDeactivated()
            }
            onLongPressed: {
                let lastActive = MenuHelper.activeMenu
                MenuHelper.closeCurrent()
                dockMenuLoader.active = true
                if (lastActive !== dockMenuLoader.item) {
                    requestShowDockMenu()
                }
            }
        }

        HoverHandler {
            id: spotlightTracker
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
            cursorShape: Qt.ArrowCursor

            onPointChanged: {
                const local = point.position
                dock.spotlightX = local.x
                dock.spotlightY = local.y
            }

            onHoveredChanged: {
                if (hovered) {
                    spotlightHideGraceTimer.stop()
                    const local = point.position
                    dock.spotlightX = local.x
                    dock.spotlightY = local.y
                    return
                }

                if (!Panel.containsMouse) {
                    spotlightHideGraceTimer.restart()
                }
            }
        }

        Connections {
            target: Panel

            function onCursorPositionChanged(cursorPosition) {
                dock.spotlightX = cursorPosition.x
                dock.spotlightY = cursorPosition.y
            }

            function onContainsMouseChanged(containsMouse) {
                if (containsMouse) {
                    spotlightHideGraceTimer.stop()
                    dock.spotlightX = Panel.cursorPosition.x
                    dock.spotlightY = Panel.cursorPosition.y
                    return
                }

                if (!spotlightTracker.hovered) {
                    spotlightHideGraceTimer.restart()
                }
            }
        }

        Timer {
            id: spotlightHideGraceTimer
            interval: dock.spotlightHideGraceInterval
            repeat: false
        }

        // TODO missing property binding to update ProxyModel's filter and sort opearation.
        Repeater {
            model: Applet.appletItems
            delegate: Item {
                property var order: model.data.dockOrder
                property bool itemVisible: model.data.shouldVisible === undefined || model.data.shouldVisible

                onItemVisibleChanged: {
                    updateAppItems()
                }
                onOrderChanged: {
                    updateAppItems()
                }
            }
        }
        //此处为边距区域的点击实践特殊处理。
        MouseArea {                                                                                                                                     
            id: leftMarginArea                                                                                                                          
            width: useColumnLayout ? parent.width : (dock.adaptiveFashionMode ? 0 : gridLayout.columnSpacing)
            height: useColumnLayout ? gridLayout.rowSpacing : parent.height                                                                             
            anchors.left: parent.left
            anchors.top: parent.top
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    dock.handleDockBackgroundClick(mouse.button)
                    return
                }

                let minOrder = Number.MAX_VALUE
                
                for (let i = 0; i < Applet.appletItems.rowCount(); i++) {
                    let itemData = Applet.appletItems.data(Applet.appletItems.index(i, 0), Qt.UserRole + 1)
                    if (itemData && itemData.dockOrder < minOrder) {
                        minOrder = itemData.dockOrder
                    }
                }
                Panel.leftEdgeClicked(minOrder)
            }                                                                       
        } 
        // TODO: remove GridLayout and use delegatechosser manager all items
        DockPartAppletModel {
            id: dockRightPartModel
            leftDockOrder: 20
            rightDockOrder: 30
            acceptItem: dock.adaptiveFashionMode ? dock.acceptAdaptiveRightDockApplet : null
        }

        Component {
            id: rightPartContent

            OverflowContainer {
                useColumnLayout: dock.useColumnLayout
                model: dockRightPartModel
            }
        }

        GridLayout {
            id: gridLayout
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: dock.adaptiveFashionMode ? 0 : 0
            columns: 1
            rows: 1
            flow: dock.useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
            property real itemMargin: Math.max((dock.dockItemIconSize / 48 * 10))
            columnSpacing: dock.adaptiveFashionMode ? dock.fashionPartSpacing : (dockLeftPartModel.count > 0 ? 10 : itemMargin)
            rowSpacing: columnSpacing

            states: State {
                name: "adaptiveFashion"
                when: dock.adaptiveFashionMode

                AnchorChanges {
                    target: gridLayout
                    anchors.top: undefined
                    anchors.right: undefined
                    anchors.bottom: undefined
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Binding {
                target: gridLayout
                property: "width"
                when: dock.adaptiveFashionMode
                value: dock.adaptiveFashionGridDisplayedWidth
            }

            Binding {
                target: gridLayout
                property: "height"
                when: dock.adaptiveFashionMode
                value: dock.adaptiveFashionGridDisplayedHeight
            }

            Item {
                id: leftMargin
                visible: !dock.adaptiveFashionMode
                implicitWidth: 0
                implicitHeight: 0
            }

            Item {
                id: dockLeftPart
                visible: dock.adaptiveFashionMode || dockLeftPartModel.count > 0
                readonly property real targetImplicitWidth: implicitWidth
                readonly property real displayedImplicitWidth: implicitWidth
                implicitWidth: dock.adaptiveFashionMode ? dock.adaptiveFashionLeftWidth : leftLoader.implicitWidth
                implicitHeight: leftLoader.implicitHeight
                width: implicitWidth
                Layout.preferredWidth: implicitWidth
                Layout.minimumWidth: implicitWidth
                Layout.maximumWidth: implicitWidth

                DockPartAppletModel {
                    id: dockLeftPartModel
                    leftDockOrder: 0
                    rightDockOrder: 10
                    acceptItem: dock.adaptiveFashionMode ? dock.acceptAdaptiveLeftDockApplet : null
                }

                Loader {
                    id: leftLoader
                    active: dockLeftPart.visible
                    sourceComponent: dock.adaptiveFashionMode ? adaptiveFashionLeftPart : legacyFashionLeftPart
                }

                Component {
                    id: legacyFashionLeftPart

                    OverflowContainer {
                        useColumnLayout: dock.useColumnLayout
                        model: dockLeftPartModel
                    }
                }

                Component {
                    id: adaptiveFashionLeftPart

                    FashionLeftDockArea {
                    }
                }
            }

            Item {  
                id: dockCenterPart
                property var taskmanagerRootObject: {
                    let applet = DS.applet("org.deepin.ds.dock.taskmanager")
                    return applet ? applet.rootObject : null
                }
                
                readonly property real taskmanagerImplicitWidth: taskmanagerRootObject ? taskmanagerRootObject.implicitWidth : 0
                readonly property real taskmanagerImplicitHeight: taskmanagerRootObject ? taskmanagerRootObject.implicitHeight : 0
                readonly property real taskmanagerAppContainerWidth: taskmanagerRootObject ? taskmanagerRootObject.appContainerWidth : 0
                readonly property real taskmanagerAppContainerHeight: taskmanagerRootObject ? taskmanagerRootObject.appContainerHeight : 0
                
                readonly property real targetImplicitWidth: implicitWidth
                readonly property real displayedImplicitWidth: implicitWidth
                implicitWidth: centerLoader.implicitWidth - taskmanagerImplicitWidth + taskmanagerAppContainerWidth
                implicitHeight: centerLoader.implicitHeight - taskmanagerImplicitHeight + taskmanagerAppContainerHeight
                onXChanged: dockCenterPartPosChanged()
                onYChanged: dockCenterPartPosChanged()
                Layout.leftMargin: !useColumnLayout && !dock.adaptiveFashionMode && Panel.itemAlignment === Dock.CenterAlignment ?
                    Math.max(0, (dock.width - dockCenterPart.implicitWidth) / 2 - (dockLeftPart.implicitWidth + 20) + Math.min((dock.width - dockCenterPart.implicitWidth) / 2 - (dockRightPart.implicitWidth + 20), 0)) : 0
                Layout.topMargin: useColumnLayout && Panel.itemAlignment === Dock.CenterAlignment ?
                    Math.max(0, (dock.height - dockCenterPart.implicitHeight) / 2 - (dockLeftPart.implicitHeight + 20) + Math.min((dock.height - dockCenterPart.implicitHeight) / 2 - (dockRightPart.implicitHeight + 20), 0)) : 0

                Behavior on Layout.leftMargin {
                    enabled: !dock.isDragging && !Applet.isResizing
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }

                Behavior on Layout.topMargin {
                    enabled: !dock.isDragging && !Applet.isResizing
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }

                OverflowContainer {
                    id: centerLoader
                    anchors.fill: parent
                    useColumnLayout: dock.useColumnLayout
                    spacing: dock.itemSpacing
                    model: dockCenterPartModel
                }

                DockPartAppletModel {
                    id: dockCenterPartModel
                    leftDockOrder: 10
                    rightDockOrder: 20
                    acceptItem: dock.adaptiveFashionMode ? dock.acceptAdaptiveCenterDockApplet : null
                    sortOrderProvider: dock.adaptiveFashionMode ? dock.adaptiveCenterSortOrder : null
                }
            }

            Item {
                id: dockTrailingBlankArea
                Layout.fillWidth: !dock.adaptiveFashionMode
                Layout.fillHeight: !dock.adaptiveFashionMode
                visible: !dock.adaptiveFashionMode
            }
        }

        MouseArea {
            id: dockBlankContextMenuArea
            anchors.fill: parent
            z: dockRightPart.z + 2
            visible: !dock.adaptiveFashionMode
            acceptedButtons: Qt.RightButton
            preventStealing: true
            propagateComposedEvents: true

            onPressed: function(mouse) {
                if (!dock.isDockBlankAreaPoint(mouse.x, mouse.y)) {
                    mouse.accepted = false
                }
            }

            onClicked: function(mouse) {
                if (!dock.isDockBlankAreaPoint(mouse.x, mouse.y)) {
                    mouse.accepted = false
                    return
                }

                dock.handleDockBackgroundClick(mouse.button)
            }
        }

        Item {
            id: dockRightPart
            visible: dockRightPartModel.count > 0
            readonly property int trailingInset: dock.adaptiveFashionMode ? DDT.TrayItemPositionManager.itemPadding + 16 : 0
            readonly property real rightContentTargetWidth: rightLoader.item
                ? (rightLoader.item.targetImplicitWidth !== undefined
                    ? rightLoader.item.targetImplicitWidth
                    : rightLoader.item.implicitWidth)
                : 0
            readonly property real targetImplicitWidth: dock.adaptiveFashionMode
                ? dock.ceilToPhysicalPixel(rightContentTargetWidth + trailingInset)
                : (rightContentTargetWidth + trailingInset)
            implicitWidth: dock.adaptiveFashionMode
                ? dock.ceilToPhysicalPixel((rightLoader.item ? rightLoader.item.implicitWidth : 0) + trailingInset)
                : ((rightLoader.item ? rightLoader.item.implicitWidth : 0) + trailingInset)
            implicitHeight: rightLoader.item ? rightLoader.item.implicitHeight : 0
            width: implicitWidth
            height: implicitHeight
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            states: State {
                name: "adaptiveFashion"
                when: dock.adaptiveFashionMode

                AnchorChanges {
                    target: dockRightPart
                    anchors.right: undefined
                    anchors.bottom: undefined
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Binding {
                target: dockRightPart
                property: "x"
                when: dock.adaptiveFashionMode
                value: dock.roundToPhysicalPixel(gridLayout.x + dock.adaptiveFashionGridDisplayedWidth + (dock.adaptiveFashionGridDisplayedWidth > 0 ? dock.fashionPartSpacing : 0))
            }

            Loader {
                id: rightLoader
                anchors.fill: parent
                active: dockRightPart.visible
                sourceComponent: rightPartContent
            }

        }

    }

    MouseArea {
        id: dragArea
        property point oldMousePos: Qt.point(0, 0)
        property int oldDockSize: 0
        property list<int> recentDeltas: []
        property int averageCount: 5
        z: dockContainer.z + 1
        hoverEnabled: true
        propagateComposedEvents: true
        enabled: !Panel.locked

        cursorShape: {
            if (Panel.locked) {
                return Qt.ArrowCursor
            }
            if (Panel.position === Dock.Top || Panel.position === Dock.Bottom) {
                return Qt.SizeVerCursor
            }
            return Qt.SizeHorCursor
        }

        onPressed: function(mouse) {
            if (Panel.locked) return
            dock.isDragging = true
            Panel.isResizing = true
            oldMousePos = mapToGlobal(mouse.x, mouse.y)
            oldDockSize = dock.preferredDockSize
            recentDeltas = []
            Panel.requestClosePopup()
            DS.grabMouse(Panel.rootObject, true)
        }

        // this used for blocking MouseEvent sent to bottom MouseArea
        onClicked: {}

        onPositionChanged: function(mouse) {
            if (Panel.locked || !dock.isDragging) return
            var newPos = mapToGlobal(mouse.x, mouse.y)
            var xChange = newPos.x - oldMousePos.x
            var yChange = newPos.y - oldMousePos.y

            if (Panel.position === Dock.Bottom || Panel.position === Dock.Top) {
                recentDeltas.push(yChange)
            } else {
                recentDeltas.push(xChange)
            }

            if (recentDeltas.length > averageCount) {
                recentDeltas.shift()
            }
            // Taking the average makes the data smooth without jumps
            var changeAverage = recentDeltas.reduce(function(acc, val) { return acc + val; }, 0) / recentDeltas.length;

            var newDockSize = 0
            if (Panel.position === Dock.Bottom) {
                newDockSize = Math.min(Math.max(oldDockSize - changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position === Dock.Top) {
                newDockSize = Math.min(Math.max(oldDockSize + changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position === Dock.Left) {
                newDockSize = Math.min(Math.max(oldDockSize + changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else {
                newDockSize = Math.min(Math.max(oldDockSize - changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            }

            newDockSize = dock.clampDockSizeByScreenLimit(newDockSize)

            if (newDockSize !== dock.preferredDockSize) {
                dock.preferredDockSize = newDockSize
                if (!dock.adaptiveFashionMode || dock.useColumnLayout) {
                    dock.dockSize = newDockSize
                } else {
                    dock.scheduleAdaptiveFashionDockSizeSync()
                }
            }

            pressedAndDragging(true)
        }

        onReleased: function(mouse) {
            if (Panel.locked) return
            dock.isDragging = false
            Applet.dockSize = dock.preferredDockSize
            itemIconSizeBase = dockItemMaxSize
            Panel.isResizing = false
            pressedAndDragging(false)
            DS.grabMouse(Panel.rootObject, false)
        }

        onCanceled: {
            dock.isDragging = false
            Panel.isResizing = false
            pressedAndDragging(false)
            DS.grabMouse(Panel.rootObject, false)
        }

        function anchorToTop() {
            anchors.top = undefined
            anchors.bottom = parent.bottom
            anchors.left = parent.left
            anchors.right = parent.right
            dragArea.height = 5
        }

        function anchorToBottom() {
            anchors.bottom = undefined
            anchors.top = parent.top
            anchors.left = parent.left
            anchors.right = parent.right
            dragArea.height = 5
        }
        function anchorToLeft() {
            anchors.left = undefined
            anchors.right = parent.right
            anchors.bottom = parent.bottom
            anchors.top = parent.top
            dragArea.width = 5
        }
        function anchorToRight() {
            anchors.right = undefined
            anchors.left = parent.left
            anchors.bottom = parent.bottom
            anchors.top = parent.top
            dragArea.width = 5
        }

    }

    function changeDragAreaAnchor() {
        switch(dock.positionForAnimation) {
        case Dock.Top: {
            dragArea.anchorToTop()
            return
        }
        case Dock.Bottom: {
            dragArea.anchorToBottom()
            return
        }
        case Dock.Left: {
            dragArea.anchorToLeft()
            return
        }
        case Dock.Right:{
            dragArea.anchorToRight()
            return
        }
        }
    }

    Connections {
        function onBeforePositionChanged(beforePosition) {
            // Stop any running animations first
            dockAnimation.stop();
            hideShowAnimation.stop();

            // Set the position for animation to old position for hide animation
            dock.positionForAnimation = beforePosition;

            // Mark that we're changing position
            dockAnimation.isPositionChanging = true;

            // Check if dock is currently hidden
            if (Panel.hideState === Dock.Hide && !dock.visible) {
                // Dock is already hidden, no need for hide animation
                // Wait for onPositionChanged to get the new position, then show
                dockAnimation.isPositionChanging = false;
            } else {
                // Start hide animation at old position
                // When animation completes, onStopped will handle show animation
                dockAnimation.startAnimation(false);
            }
        }

        function onPositionChanged() {
            Panel.requestClosePopup()

            // If dock was hidden when position change started, show it now at new position
            if (!dockAnimation.isPositionChanging && !dock.visible) {
                dock.positionForAnimation = Panel.position;

                // Set transform to hidden position before showing
                dockAnimation.setTransformToHiddenPosition();

                dockAnimation.startAnimation(true);
            }
        }
        function onViewModeChanged() {
            updateAppItems()
            dock.syncDockPresentationForCurrentMode()
        }
        function onDockSizeChanged() {
            dock.preferredDockSize = Panel.dockSize
            if (!dock.adaptiveFashionMode || dock.useColumnLayout) {
                dock.dockSize = Panel.dockSize
            }
            dock.scheduleAdaptiveFashionDockSizeSync()
        }

        function onHideStateChanged() {
            if (Panel.hideState === Dock.Hide) {
                hideTimer.running = true
            } else {
                // Only restart animation if not already running or if visible state doesn't match
                if (!hideShowAnimation.running || !dock.visible) {
                    hideShowAnimation.restart()
                }
            }
        }
        function onRequestClosePopup() {
            let popup = Panel.popupWindow
            DS.closeChildrenWindows(popup)
            if (popup && popup.visible)
                popup.close()
        }
        function onDockScreenChanged() {
            // Close all popups when dock moves to another screen
            Panel.requestClosePopup()
        }

        target: Panel
    }

    Connections {
        function onMenuClosed() {
            // 当右键菜单关闭时，根据当前隐藏模式重新启动隐藏计时器
            if (Panel.hideMode === Dock.KeepHidden
                || Panel.hideMode === Dock.SmartHide
                || Panel.hideState === Dock.Hide) {
                hideTimer.running = true
            }
        }

        target: MenuHelper
    }

    function position2Anchors(position) {
        switch (position) {
        case Dock.Top:
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop
        case Dock.Right:
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorRight
        case Dock.Bottom:
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorBottom
        case Dock.Left:
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorLeft
        }
    }

    // can not move into DockCompositor
    // Panel get a object instead of ds::dock::DockPanel during DockCompositor creating
    Binding {
        target: Panel
        property: "compositorReady"
        value: DockCompositor.compositor.created
        when: DockCompositor.compositor.created
    }

    Component.onCompleted: {
        Panel.toolTipWindow.windowThemeType = Qt.binding(function(){
            return Panel.colorTheme
        })

        Panel.popupWindow.windowThemeType = Qt.binding(function(){
            return Panel.colorTheme
        })

        Panel.menuWindow.windowThemeType = Qt.binding(function(){
            return Panel.colorTheme
        })

        DockCompositor.dockColorTheme = Qt.binding(function(){
            return Panel.colorTheme
        })

        DockCompositor.dockPosition = Qt.binding(function(){
            return dock.positionForAnimation
        })

        DockCompositor.dockSize = Qt.binding(function(){
            return Qt.size(Panel.frontendWindowRect.width, Panel.frontendWindowRect.height)
        })

        DockCompositor.panelScale = Qt.binding(function(){
            return Panel.devicePixelRatio
        })

        dock.itemIconSizeBase = dock.dockItemMaxSize
        dock.syncDockPresentationForCurrentMode()
    }
}
