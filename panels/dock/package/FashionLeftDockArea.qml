// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15
import QtQuick.Window 2.15
import Qt5Compat.GraphicalEffects

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Control {
    id: root

    readonly property int dockSize: Panel.rootObject.dockSize
    readonly property int hoverInset: 3
    readonly property real taskbarRadius: Panel.rootObject && Panel.rootObject.fashionBackgroundRadius !== undefined
        ? Panel.rootObject.fashionBackgroundRadius
        : Math.round(dockSize / 5)
    readonly property real hoverBackgroundRadius: Math.max(0, taskbarRadius - hoverInset)
    readonly property int hoverFadeDuration: 150
    readonly property int pageSlideDuration: 260
    readonly property int leftContentPadding: hoverInset
    readonly property int rightContentPadding: Math.max(10, Math.round(dockSize * 0.2))
    readonly property int widthExpansion: 0
    readonly property int foregroundContentOffset: 8
    readonly property int systemForegroundContentOffset: 10
    readonly property int textVerticalLift: 1
    readonly property int weatherTextVerticalLift: 2
    readonly property int verticalInset: Math.max(5, Math.round(dockSize * 0.16))
    readonly property int pageContentHeight: Math.max(24, dockSize - verticalInset * 2)
    readonly property int pageSpacing: Math.max(8, Math.round(dockSize * 0.14))
    readonly property int tightSpacing: Math.max(1, Math.round(dockSize * 0.04))
    readonly property int mediumIconSize: Math.max(18, Math.round(pageContentHeight * 0.62))
    readonly property int pluginRowLeftMargin: root.leftContentPadding + root.foregroundContentOffset
    readonly property int pluginLeadingIconSize: root.mediumIconSize
    readonly property int compactPluginLeadingSlotSize: Math.max(root.pluginLeadingIconSize, root.pageContentHeight + 2)
    readonly property int pluginLeadingSlotSize: root.singleLineCompactMode
        ? root.compactPluginLeadingSlotSize
        : Math.max(32, root.pluginLeadingIconSize)
    readonly property int musicArtSize: root.pluginLeadingIconSize
    readonly property real musicArtRadius: 4
    readonly property int musicControlIconSize: 16
    readonly property int musicControlButtonSize: root.singleLineCompactMode ? 26 : 30
    readonly property real musicControlButtonRadius: musicControlButtonSize / 2
    readonly property int musicControlSpacing: Math.max(3, Math.round(pageContentHeight * 0.07))
    readonly property int musicTitleButtonSpacing: Math.max(2, root.tightSpacing - 1)
    readonly property int musicTextSpacing: 1
    readonly property int musicRowSpacing: root.singleLineCompactMode ? Math.max(8, root.pageSpacing - 1) : 10
    readonly property bool singleLineCompactMode: root.pageContentHeight <= 28
    readonly property bool showSecondaryLine: !root.singleLineCompactMode
    readonly property int sharedPageVerticalOffset: 0
    readonly property int weatherPageVerticalOffset: 0
    readonly property int aiPageVerticalOffset: 0
    readonly property int musicPageVerticalOffset: 0
    readonly property int sharedTextVerticalOffset: root.showSecondaryLine ? -root.textVerticalLift : 0
    readonly property int weatherInfoVerticalOffset: root.showSecondaryLine
        ? -root.weatherTextVerticalLift
        : 0
    readonly property int musicControlsWidth: root.musicControlButtonSize * 3 + root.musicControlSpacing * 2
    readonly property real musicArtworkDevicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int timeFontSize: Math.max(14, Math.round(pageContentHeight * 0.47))
    readonly property int temperatureUnitFontSize: timeFontSize - 2
    readonly property int headlineFontSize: Math.max(13, Math.round(pageContentHeight * 0.4))
    readonly property int metricFontSize: Math.max(13, Math.round(pageContentHeight * 0.4))
    readonly property int secondaryFontSize: Math.max(9, Math.round(pageContentHeight * 0.24))
    readonly property int captionFontSize: Math.max(9, Math.round(pageContentHeight * 0.2))
    readonly property int weatherSecondaryFontSize: secondaryFontSize + 2
    readonly property int monitorDetailFontSize: captionFontSize + 2
    readonly property int aiProcessFontSize: captionFontSize + 2
    readonly property int aiPrimaryCountFontSize: root.metricFontSize + 1
    readonly property int aiSecondaryCountFontSize: root.captionFontSize + 2
    readonly property int aiStatusBarHeight: 2
    readonly property int aiStatusBarTopMargin: root.aiShowSecondaryLine ? 1 : 0
    readonly property int aiStatusBarAnimationDuration: 1150
    readonly property bool aiShowSecondaryLine: root.pageContentHeight > 34
    readonly property int aiTextVerticalOffset: root.aiShowSecondaryLine ? -root.textVerticalLift : 0
    readonly property int aiMetricColumnSpacing: root.aiShowSecondaryLine ? root.tightSpacing : 0
    readonly property color aiIconTintColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.92) : Qt.rgba(0, 0, 0, 0.86)
    readonly property int weatherTextSpacing: root.tightSpacing - 2
    readonly property bool musicPageVisible: provider.musicAvailable
    readonly property bool aiPageVisible: provider.aiRunningCount > 0 && aiEntries.length > 0
    readonly property var pageIds: {
        const ids = ["weather"]
        if (aiPageVisible) {
            ids.push("ai")
        }
        if (musicPageVisible) {
            ids.push("music")
        }
        ids.push("mail")
        ids.push("system")
        return ids
    }
    readonly property int pageCount: pageIds.length
    readonly property int pageIndicatorSize: 2
    readonly property int pageIndicatorSpacing: 4
    readonly property int pageIndicatorRightInset: Math.max(4, root.hoverInset + 3)
    readonly property real hiddenPageOffset: root.dockSize + Math.max(8, root.verticalInset * 2)
    readonly property bool autoRotateEnabled: false
    readonly property int autoRotateInterval: 4000
    readonly property int maxMusicTitleWidth: Math.max(root.musicControlsWidth + Math.max(6, root.musicControlSpacing * 2),
        Math.round(dockSize * 1.58))
    readonly property string weatherTraySurfaceId: "deepin-weather::weather"
    property string currentPageId: "weather"
    property string manualPageId: "weather"
    property string musicReturnPageId: "weather"
    property string mailReturnPageId: "weather"
    property bool musicAutoActive: false
    property bool mailAutoActive: false
    property bool previousMusicAvailable: provider.musicAvailable
    property int previousMailUnreadCount: provider.mailUnreadCount
    property string transitionFromPageId: ""
    property string transitionToPageId: ""
    property int transitionDirection: 1
    property point lastSpotlightPoint: Qt.point(0, 0)
    property real musicHoverProgress: rootHoverHandler.hovered ? 1 : 0
    property real wheelDeltaAccumulator: 0
    property int wheelDirectionLatch: 0
    property bool wheelStepTriggeredInGesture: false
    readonly property int wheelStepThreshold: 120
    readonly property int wheelGestureResetInterval: 180
    readonly property int currentIndex: Math.max(0, root.pageIds.indexOf(root.normalizedPageId(root.currentPageId)))
    readonly property int indicatorIndex: transitionToPageId.length > 0
        ? Math.max(0, root.pageIds.indexOf(root.normalizedPageId(root.transitionToPageId)))
        : root.currentIndex
    readonly property string tooltipPageId: transitionToPageId.length > 0
        ? root.normalizedPageId(root.transitionToPageId)
        : root.normalizedPageId(root.currentPageId)
    readonly property var aiEntries: provider.aiToolEntries || []
    readonly property int aiEntryCount: Math.min(2, aiEntries.length)
    readonly property bool aiDualColumnMode: aiEntryCount > 1
    readonly property string aiPrimaryDisplayToolId: provider.aiPrimaryToolId && provider.aiPrimaryToolId.length > 0
        ? provider.aiPrimaryToolId
        : ((aiEntries.length > 0 && aiEntries[0].toolId) ? String(aiEntries[0].toolId) : "")
    readonly property string currentTooltipText: {
        switch (root.tooltipPageId) {
        case "ai":
            return root.aiTooltipText()
        case "music":
            return root.musicTooltipText()
        case "mail":
            return root.mailTooltipText()
        case "system":
            return root.systemTooltipText()
        default:
            return root.weatherTooltipText()
        }
    }

    Behavior on musicHoverProgress {
        NumberAnimation {
            duration: root.hoverFadeDuration + 70
            easing.type: Easing.OutCubic
        }
    }

    readonly property color primaryTextColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(0, 0, 0, 0.92)
    readonly property color secondaryTextColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.68) : Qt.rgba(0, 0, 0, 0.58)
    readonly property color musicControlForegroundColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 1) : Qt.rgba(0, 0, 0, 1)
    readonly property string dataFontFamily: DS.dataFontFamily.length > 0 ? DS.dataFontFamily : D.DTK.fontManager.t6.family
    readonly property int taskbarWidth: Panel.rootObject && Panel.rootObject.adaptiveFashionLeftWidth !== undefined
        ? Panel.rootObject.adaptiveFashionLeftWidth
        : 160
    implicitWidth: taskbarWidth
    width: taskbarWidth
    implicitHeight: dockSize
    padding: 0

    FashionLeftPluginProvider {
        id: provider
    }

    Item {
        id: textProbeLayer
        visible: false
        width: 0
        height: 0

        Text {
            id: systemMetricValueProbe
            text: "100%"
            font.family: root.dataFontFamily
            font.pixelSize: root.metricFontSize
            font.weight: Font.DemiBold
            renderType: Text.NativeRendering
        }

        Text {
            id: systemTrafficProbe
            text: "999.9mb/s ↓"
            font.family: root.dataFontFamily
            font.pixelSize: root.monitorDetailFontSize
            renderType: Text.NativeRendering
        }
    }

    function mapSpotlightPoint(localPoint) {
        const point = localPoint || Qt.point(width / 2, height / 2)
        return mapToItem(null, point.x, point.y)
    }

    function updateSpotlight(localPoint) {
        lastSpotlightPoint = mapSpotlightPoint(localPoint)
        Panel.reportMousePresence(true, lastSpotlightPoint)
    }

    function clearSpotlight() {
        Panel.reportMousePresence(false, lastSpotlightPoint)
    }

    function prepareWeatherTraySurface() {
        const surface = DockCompositor.findSurface(root.weatherTraySurfaceId)
        if (!surface) {
            return
        }

        const panelPoint = weatherPage.mapToItem(null, 0, 0)
        const globalPoint = weatherPage.mapToGlobal(0, 0)
        surface.updatePluginGeometry(Qt.rect(Math.round(panelPoint.x),
                                             Math.round(panelPoint.y),
                                             Math.round(weatherPage.width),
                                             Math.round(weatherPage.height)))
        surface.setGlobalPos(Qt.point(Math.round(globalPoint.x), Math.round(globalPoint.y)))
    }

    function resetWheelGesture() {
        root.wheelDeltaAccumulator = 0
        root.wheelDirectionLatch = 0
        root.wheelStepTriggeredInGesture = false
    }

    function handleWheelPaging(deltaY) {
        if (deltaY === 0) {
            return
        }

        const direction = deltaY < 0 ? 1 : -1
        if (root.wheelDirectionLatch !== 0 && root.wheelDirectionLatch !== direction) {
            root.resetWheelGesture()
        }

        root.wheelDirectionLatch = direction
        wheelGestureResetTimer.restart()

        if (root.wheelStepTriggeredInGesture
                || pageTransitionAnimation.running
                || root.transitionToPageId.length > 0) {
            return
        }

        root.wheelDeltaAccumulator += Math.abs(deltaY)
        if (root.wheelDeltaAccumulator < root.wheelStepThreshold) {
            return
        }

        root.wheelStepTriggeredInGesture = true
        root.wheelDeltaAccumulator = 0
        root.step(direction)
    }

    function visiblePageIds() {
        return root.pageIds
    }

    function normalizedPageId(pageId) {
        return root.pageIds.indexOf(pageId) >= 0 ? pageId : "weather"
    }

    function pageItemForId(pageId) {
        switch (pageId) {
        case "ai":
            return aiPage
        case "music":
            return musicPage
        case "mail":
            return mailPage
        case "system":
            return systemPage
        default:
            return weatherPage
        }
    }

    function canAutoRotate() {
        return root.autoRotateEnabled
            && root.pageCount > 1
            && !rootHoverHandler.hovered
            && !root.musicAutoActive
            && !root.mailAutoActive
    }

    function restartAutoRotateTimer() {
        autoRotateTimer.stop()
        if (root.canAutoRotate()) {
            autoRotateTimer.start()
        }
    }

    function syncPageLayout() {
        const visibleIds = root.visiblePageIds()
        const inTransition = pageTransitionAnimation.running || root.transitionToPageId.length > 0
        const activePageId = root.normalizedPageId(root.currentPageId)
        const items = [weatherPage, aiPage, musicPage, mailPage, systemPage]
        for (let index = 0; index < items.length; ++index) {
            const item = items[index]
            const available = visibleIds.indexOf(item.pageId) >= 0
            const active = inTransition
                ? (item.pageId === root.transitionFromPageId || item.pageId === root.transitionToPageId)
                : item.pageId === activePageId
            item.visible = available && active
            if (!item.visible) {
                item.y = root.hiddenPageOffset
            } else if (!inTransition) {
                item.y = 0
            }
        }
    }

    function completePageTransition() {
        if (root.transitionToPageId.length > 0) {
            root.currentPageId = root.normalizedPageId(root.transitionToPageId)
        }

        root.transitionFromPageId = ""
        root.transitionToPageId = ""
        root.transitionDirection = 1
        root.syncPageLayout()
        root.restartAutoRotateTimer()
    }

    function transitionDirectionForTarget(fromIndex, targetIndex) {
        const ids = root.visiblePageIds()
        if (ids.length <= 1) {
            return 1
        }

        if (fromIndex === ids.length - 1 && targetIndex === 0) {
            return 1
        }

        if (fromIndex === 0 && targetIndex === ids.length - 1) {
            return -1
        }

        return targetIndex >= fromIndex ? 1 : -1
    }

    function showPage(pageId, animated) {
        const shouldAnimate = animated === undefined ? true : animated
        const ids = root.visiblePageIds()
        const targetPageId = root.normalizedPageId(pageId)
        const targetIndex = ids.indexOf(targetPageId)
        const fromPageId = root.transitionToPageId.length > 0
            ? root.normalizedPageId(root.transitionToPageId)
            : root.normalizedPageId(root.currentPageId)
        const fromIndex = Math.max(0, ids.indexOf(fromPageId))

        autoRotateTimer.stop()

        if (targetIndex < 0) {
            return
        }

        if (pageTransitionAnimation.running) {
            pageTransitionAnimation.stop()
        }

        if (!shouldAnimate || ids.length < 2 || targetPageId === fromPageId) {
            root.currentPageId = targetPageId
            root.transitionFromPageId = ""
            root.transitionToPageId = ""
            root.transitionDirection = 1
            root.syncPageLayout()
            root.restartAutoRotateTimer()
            return
        }

        const fromItem = root.pageItemForId(fromPageId)
        const toItem = root.pageItemForId(targetPageId)
        if (!fromItem || !toItem) {
            root.currentPageId = targetPageId
            root.transitionFromPageId = ""
            root.transitionToPageId = ""
            root.transitionDirection = 1
            root.syncPageLayout()
            return
        }

        root.transitionFromPageId = fromPageId
        root.transitionToPageId = targetPageId
        root.transitionDirection = root.transitionDirectionForTarget(fromIndex, targetIndex)
        root.syncPageLayout()

        fromItem.y = 0
        toItem.y = root.transitionDirection > 0 ? root.height : -root.height

        transitionFromAnimation.target = fromItem
        transitionFromAnimation.from = 0
        transitionFromAnimation.to = -root.transitionDirection * root.height

        transitionToAnimation.target = toItem
        transitionToAnimation.from = root.transitionDirection > 0 ? root.height : -root.height
        transitionToAnimation.to = 0

        pageTransitionAnimation.start()
    }

    function showManualPage(pageId) {
        root.dismissAutoFocus()
        const nextPageId = root.normalizedPageId(pageId)
        root.manualPageId = nextPageId
        root.showPage(nextPageId)
    }

    function step(delta) {
        const ids = root.visiblePageIds()
        const nextIndex = (root.currentIndex + delta + ids.length) % ids.length
        root.dismissAutoFocus()
        root.manualPageId = ids[nextIndex]
        root.showPage(ids[nextIndex])
    }

    function weatherTemperatureValue(text) {
        if (!text) {
            return "--"
        }

        return text.replace(/\s*°[CF]?$/, "")
    }

    function musicControlIconSource(iconName) {
        const themeIconSource = provider.musicControlThemeIconSource(iconName, Panel.colorTheme === Dock.Dark)
        return themeIconSource && themeIconSource.length > 0
            ? themeIconSource
            : Qt.resolvedUrl("icons/" + iconName + ".svg")
    }

    function weatherTooltipText() {
        const cityText = provider.weatherCityText && provider.weatherCityText.length > 0 ? provider.weatherCityText : qsTr("当前城市")
        const temperatureText = provider.weatherTemperatureText && provider.weatherTemperatureText.length > 0
            ? provider.weatherTemperatureText.replace(/°$/, "°C")
            : "--°C"
        return cityText + " " + temperatureText
    }

    function musicTooltipText() {
        const titleText = provider.musicTitleText || ""
        const appText = provider.musicAppName || ""
        if (titleText.length > 0 && appText.length > 0) {
            return titleText + " - " + appText
        }

        return titleText.length > 0 ? titleText : appText
    }

    function aiIconSource(toolId) {
        const normalizedToolId = toolId ? String(toolId).toLowerCase() : ""
        switch (normalizedToolId) {
        case "codex":
        case "openai":
            return Qt.resolvedUrl("icons/openai.svg")
        case "claude":
        case "claude-code":
            return Qt.resolvedUrl("icons/claude.svg")
        case "doubao":
            return Qt.resolvedUrl("icons/doubao.svg")
        case "gemini":
            return Qt.resolvedUrl("icons/gemini.svg")
        case "qwen":
            return Qt.resolvedUrl("icons/qwen.svg")
        case "aistudio":
        case "google-ai-studio":
            return Qt.resolvedUrl("icons/aistudio.svg")
        default:
            return Qt.resolvedUrl("icons/ai.svg")
        }
    }

    function aiTooltipText() {
        if (root.aiEntries.length <= 0) {
            return qsTr("AI")
        }

        let parts = []
        for (let index = 0; index < Math.min(2, root.aiEntries.length); ++index) {
            const entry = root.aiEntries[index]
            const progressText = entry && entry.progressText ? String(entry.progressText) : "0/0"
            const processLabel = entry && entry.processLabel ? String(entry.processLabel) : "ai"
            parts.push(progressText + qsTr(" 条任务") + " · " + processLabel)
        }
        return parts.join(" · ")
    }

    function mailTooltipText() {
        const clientText = provider.mailClientName && provider.mailClientName.length > 0 ? provider.mailClientName : qsTr("邮箱")
        return qsTr("%1 %2 封未读").arg(clientText).arg(provider.mailUnreadCount)
    }

    function systemTooltipText() {
        return qsTr("CPU %1% · 内存 %2%").arg(provider.cpuUsage).arg(provider.memoryUsage)
    }

    function updateToolTipGeometry() {
        const point = root.mapToItem(null, root.width / 2, root.height / 2)
        pageToolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, pageToolTip.width, pageToolTip.height)
    }

    function showToolTip() {
        if (!rootHoverHandler.hovered || !root.currentTooltipText || root.currentTooltipText.length === 0) {
            return
        }

        pageToolTip.text = root.currentTooltipText
        root.updateToolTipGeometry()
        pageToolTip.open()
    }

    function hideToolTip() {
        if (toolTipShowTimer.running) {
            toolTipShowTimer.stop()
        }
        pageToolTip.close()
    }

    function dismissAutoFocus() {
        root.musicAutoActive = false
        root.mailAutoActive = false
    }

    function handleMusicAvailabilityChange() {
        if (provider.musicAvailable === root.previousMusicAvailable) {
            return
        }

        if (provider.musicAvailable) {
            if (!root.mailAutoActive && root.currentPageId !== "music") {
                root.musicReturnPageId = root.normalizedPageId(root.currentPageId)
                root.musicAutoActive = true
                root.showPage("music")
            }
        } else {
            if (root.mailAutoActive && root.mailReturnPageId === "music") {
                root.mailReturnPageId = root.normalizedPageId(root.musicReturnPageId)
            }

            const fallbackPageId = root.normalizedPageId(root.musicAutoActive ? root.musicReturnPageId : root.manualPageId)
            if (root.currentPageId === "music") {
                root.showPage(fallbackPageId)
            }

            if (root.manualPageId === "music") {
                root.manualPageId = "weather"
            }

            root.musicAutoActive = false
        }

        root.previousMusicAvailable = provider.musicAvailable
    }

    function handleMailUnreadCountChange() {
        const unreadIncreased = provider.mailUnreadCount > root.previousMailUnreadCount
        if (unreadIncreased && root.currentPageId !== "mail") {
            root.mailReturnPageId = root.normalizedPageId(root.currentPageId)
            root.mailAutoActive = true
            root.showPage("mail")
        }

        root.previousMailUnreadCount = provider.mailUnreadCount
    }

    function completeMailAutoFocus() {
        if (!root.mailAutoActive) {
            return
        }

        root.mailAutoActive = false
        root.showPage(root.mailReturnPageId)
    }

    HoverHandler {
        id: rootHoverHandler
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus

        onPointChanged: {
            if (hovered) {
                spotlightClearTimer.stop()
                root.updateSpotlight(rootHoverHandler.point.position)
            }
        }

        onHoveredChanged: {
            if (hovered) {
                spotlightClearTimer.stop()
                root.updateSpotlight()
                autoRotateTimer.stop()
                toolTipShowTimer.stop()
                toolTipShowTimer.start()
            } else {
                spotlightClearTimer.restart()
                root.hideToolTip()
                root.restartAutoRotateTimer()
            }
        }
    }

    Timer {
        id: spotlightClearTimer
        interval: 70
        repeat: false
        onTriggered: {
            if (!rootHoverHandler.hovered) {
                root.clearSpotlight()
            }
        }
    }

    Connections {
        target: provider

        function onMusicStateChanged() {
            root.handleMusicAvailabilityChange()
            if (rootHoverHandler.hovered) {
                if (pageToolTip.toolTipVisible) {
                    root.showToolTip()
                } else {
                    toolTipShowTimer.stop()
                    toolTipShowTimer.start()
                }
            }
        }

        function onMailStateChanged() {
            root.handleMailUnreadCountChange()
            if (rootHoverHandler.hovered && root.currentPageId === "mail") {
                if (pageToolTip.toolTipVisible) {
                    root.showToolTip()
                } else {
                    toolTipShowTimer.stop()
                    toolTipShowTimer.start()
                }
            }
        }

        function onWeatherChanged() {
            if (rootHoverHandler.hovered && root.currentPageId === "weather") {
                if (pageToolTip.toolTipVisible) {
                    root.showToolTip()
                } else {
                    toolTipShowTimer.stop()
                    toolTipShowTimer.start()
                }
            }
        }

        function onSystemStatsChanged() {
            if (rootHoverHandler.hovered && root.currentPageId === "system") {
                if (pageToolTip.toolTipVisible) {
                    root.showToolTip()
                }
            }
        }

        function onAiStateChanged() {
            if (rootHoverHandler.hovered && root.currentPageId === "ai") {
                if (pageToolTip.toolTipVisible) {
                    root.showToolTip()
                } else {
                    toolTipShowTimer.stop()
                    toolTipShowTimer.start()
                }
            }
        }
    }

    onCurrentPageIdChanged: {
        if (!rootHoverHandler.hovered) {
            root.restartAutoRotateTimer()
            return
        }

        if (pageToolTip.toolTipVisible) {
            root.showToolTip()
        } else {
            toolTipShowTimer.stop()
            toolTipShowTimer.start()
        }

        root.restartAutoRotateTimer()
    }

    onCurrentIndexChanged: {
        if (!pageTransitionAnimation.running && root.transitionToPageId.length === 0) {
            root.syncPageLayout()
        }
    }

    onTransitionToPageIdChanged: {
        if (!rootHoverHandler.hovered) {
            return
        }

        if (transitionToPageId.length > 0 || pageToolTip.toolTipVisible) {
            root.showToolTip()
        } else {
            toolTipShowTimer.stop()
            toolTipShowTimer.start()
        }
    }

    onPageCountChanged: {
        root.syncPageLayout()
        root.restartAutoRotateTimer()
    }
    onHeightChanged: root.syncPageLayout()
    onMusicAutoActiveChanged: root.restartAutoRotateTimer()
    onMailAutoActiveChanged: root.restartAutoRotateTimer()

    background: Item {
        AppletItemBackground {
            x: root.hoverInset
            y: root.hoverInset
            width: Math.max(0, parent.width - root.hoverInset)
            height: Math.max(0, parent.height - root.hoverInset * 2)
            radius: root.hoverBackgroundRadius
            enabled: false
            opacity: rootHoverHandler.hovered ? 1 : 0
            D.ColorSelector.hovered: rootHoverHandler.hovered

            Behavior on opacity {
                NumberAnimation {
                    duration: root.hoverFadeDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Item {
            anchors.right: parent.right
            anchors.rightMargin: root.pageIndicatorRightInset
            anchors.verticalCenter: parent.verticalCenter
            visible: rootHoverHandler.hovered && root.pageCount > 1
            implicitWidth: root.pageIndicatorSize
            implicitHeight: indicatorColumn.implicitHeight

            Column {
                id: indicatorColumn
                anchors.centerIn: parent
                spacing: root.pageIndicatorSpacing

                Repeater {
                    model: root.pageCount

                    Rectangle {
                        width: root.pageIndicatorSize
                        height: root.pageIndicatorSize
                        radius: width / 2
                        color: root.primaryTextColor
                        opacity: index === root.indicatorIndex ? 1 : 0.35
                    }
                }
            }
        }
    }

    PanelToolTip {
        id: pageToolTip
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    Timer {
        id: autoRotateTimer
        interval: root.autoRotateInterval
        repeat: true
        running: false
        onTriggered: {
            if (!root.canAutoRotate() || pageTransitionAnimation.running || root.transitionToPageId.length > 0) {
                return
            }

            root.step(1)
        }
    }

    Timer {
        id: toolTipShowTimer
        interval: 50
        onTriggered: root.showToolTip()
    }

    Timer {
        id: wheelGestureResetTimer
        interval: root.wheelGestureResetInterval
        repeat: false
        onTriggered: root.resetWheelGesture()
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function(event) {
            const deltaY = event.angleDelta.y !== 0 ? event.angleDelta.y : event.pixelDelta.y
            root.handleWheelPaging(deltaY)

            if (rootHoverHandler.hovered) {
                root.showToolTip()
            }
            event.accepted = true
        }
    }

    component PageShell: Item {
        id: pageShell

        property string pageId: ""

        signal clicked(real globalX, real globalY)

        width: root.width
        height: root.height

        MouseArea {
            id: pageMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                const globalPos = pageMouseArea.mapToGlobal(mouse.x, mouse.y)
                root.hideToolTip()
                pageShell.clicked(globalPos.x, globalPos.y)
            }
        }
    }

    component MusicActionButton: Item {
        id: actionButton

        property string iconName: ""
        property bool actionEnabled: false
        property int layoutIndex: 0
        property int hoverDelay: 0
        property bool hoverTarget: rootHoverHandler.hovered
        property real revealProgress: hoverTarget ? 1 : 0
        readonly property real collapsedX: (root.musicControlsWidth - width) / 2
        readonly property real expandedX: layoutIndex * (root.musicControlButtonSize + root.musicControlSpacing)
        readonly property bool interactive: actionEnabled && revealProgress > 0.55
        signal clicked()

        implicitWidth: root.musicControlButtonSize
        implicitHeight: root.musicControlButtonSize
        x: collapsedX + (expandedX - collapsedX) * revealProgress
        y: 0
        scale: 0.7 + revealProgress * 0.3
        opacity: revealProgress
        z: layoutIndex === 1 ? 3 : 2 - Math.abs(layoutIndex - 1)

        onHoverTargetChanged: revealProgress = hoverTarget ? 1 : 0
        Component.onCompleted: revealProgress = hoverTarget ? 1 : 0

        Behavior on revealProgress {
            SequentialAnimation {
                PauseAnimation {
                    duration: actionButton.hoverTarget ? actionButton.hoverDelay : 0
                }
                NumberAnimation {
                    duration: root.hoverFadeDuration + 110
                    easing.type: Easing.OutCubic
                }
            }
        }

        HoverHandler {
            id: actionHoverHandler
            enabled: actionButton.interactive
        }

        Rectangle {
            anchors.fill: parent
            radius: root.musicControlButtonRadius
            color: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.08)
            opacity: actionHoverHandler.hovered ? actionButton.revealProgress : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: root.hoverFadeDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Image {
            id: actionIconImage
            anchors.centerIn: parent
            width: root.musicControlIconSize
            height: root.musicControlIconSize
            source: root.musicControlIconSource(actionButton.iconName)
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: false
        }

        ColorOverlay {
            anchors.fill: actionIconImage
            source: actionIconImage
            color: root.musicControlForegroundColor
            opacity: actionButton.actionEnabled ? 1 : 0.38
        }

        MouseArea {
            anchors.fill: parent
            enabled: actionButton.interactive
            hoverEnabled: true
            cursorShape: actionButton.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: function(mouse) {
                mouse.accepted = true
                if (actionButton.actionEnabled) {
                    actionButton.clicked()
                }
            }
        }
    }

    ParallelAnimation {
        id: pageTransitionAnimation

        NumberAnimation {
            id: transitionFromAnimation
            property: "y"
            duration: root.pageSlideDuration
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            id: transitionToAnimation
            property: "y"
            duration: root.pageSlideDuration
            easing.type: Easing.OutCubic
        }

        onStopped: root.completePageTransition()
    }

    contentItem: Item {
        clip: true

        Component.onCompleted: root.syncPageLayout()

        PageShell {
            id: weatherPage
            pageId: "weather"

            implicitWidth: weatherRow.implicitWidth
            onClicked: function(globalX, globalY) {
                const taskbarTopLeft = root.mapToGlobal(0, 0)
                root.prepareWeatherTraySurface()
                provider.openWeatherPopup(Math.round(taskbarTopLeft.x),
                                          Math.round(taskbarTopLeft.y),
                                          Math.round(globalX),
                                          Math.round(globalY))
            }

            RowLayout {
                id: weatherRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: root.weatherPageVerticalOffset
                anchors.leftMargin: root.pluginRowLeftMargin
                anchors.rightMargin: root.rightContentPadding
                height: root.pageContentHeight
                spacing: root.pageSpacing

                Item {
                    implicitWidth: root.pluginLeadingSlotSize
                    implicitHeight: implicitWidth
                    Layout.alignment: Qt.AlignVCenter

                    Image {
                        anchors.centerIn: parent
                        width: root.pluginLeadingIconSize
                        height: root.pluginLeadingIconSize
                        source: provider.weatherIconSource && provider.weatherIconSource.toString().length > 0
                            ? provider.weatherIconSource
                            : Qt.resolvedUrl("icons/weather-none-available.svg")
                        sourceSize: Qt.size(width, height)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        asynchronous: true
                    }
                }

                Item {
                    implicitWidth: weatherTextColumn.implicitWidth
                    implicitHeight: weatherTextColumn.implicitHeight
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.maximumHeight: root.pageContentHeight

                    ColumnLayout {
                        id: weatherTextColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: root.weatherInfoVerticalOffset
                        spacing: root.weatherTextSpacing

                        Item {
                            implicitWidth: temperatureValueText.implicitWidth + temperatureUnitText.implicitWidth
                            implicitHeight: temperatureValueText.implicitHeight

                            Text {
                                id: temperatureValueText
                                text: root.weatherTemperatureValue(provider.weatherTemperatureText)
                                color: root.primaryTextColor
                                font.family: root.dataFontFamily
                                font.pixelSize: root.timeFontSize
                                font.weight: Font.DemiBold
                                renderType: Text.NativeRendering
                            }

                            Text {
                                id: temperatureUnitText
                                anchors.left: temperatureValueText.right
                                anchors.baseline: temperatureValueText.baseline
                                text: "°C"
                                color: root.primaryTextColor
                                font.family: root.dataFontFamily
                                font.pixelSize: root.temperatureUnitFontSize
                                font.weight: Font.DemiBold
                                renderType: Text.NativeRendering
                            }
                        }

                        Text {
                            text: provider.weatherSummaryText
                            color: root.secondaryTextColor
                            font.pixelSize: root.weatherSecondaryFontSize
                            renderType: Text.NativeRendering
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            visible: root.showSecondaryLine
                        }
                    }
                }
            }
        }

        PageShell {
            id: aiPage
            pageId: "ai"

            implicitWidth: aiRow.implicitWidth
            onClicked: provider.openAiClientHost()

            RowLayout {
                id: aiRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: root.aiPageVerticalOffset
                anchors.leftMargin: root.pluginRowLeftMargin
                anchors.rightMargin: root.rightContentPadding
                height: root.pageContentHeight
                spacing: root.aiDualColumnMode ? 0 : root.pageSpacing

                Item {
                    implicitWidth: root.aiDualColumnMode ? 0 : root.pluginLeadingSlotSize
                    implicitHeight: implicitWidth
                    Layout.alignment: Qt.AlignVCenter
                    visible: !root.aiDualColumnMode

                    Image {
                        id: aiPrimaryIcon
                        anchors.centerIn: parent
                        width: root.pluginLeadingIconSize
                        height: root.pluginLeadingIconSize
                        source: root.aiIconSource(root.aiPrimaryDisplayToolId)
                        sourceSize: Qt.size(width, height)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        asynchronous: true
                        visible: false
                    }

                    ColorOverlay {
                        anchors.fill: aiPrimaryIcon
                        source: aiPrimaryIcon
                        color: root.aiIconTintColor
                        visible: aiPrimaryIcon.source !== ""
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: root.pluginLeadingIconSize
                        height: root.pluginLeadingIconSize
                        radius: width / 3
                        color: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.06)
                        border.width: 1
                        border.color: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.08)
                        visible: aiPrimaryIcon.source === ""
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "AI"
                        color: root.primaryTextColor
                        font.pixelSize: root.captionFontSize + 1
                        font.weight: Font.DemiBold
                        renderType: Text.NativeRendering
                        visible: aiPrimaryIcon.source === ""
                    }
                }

                Item {
                    implicitWidth: aiMetricsRow.implicitWidth
                    implicitHeight: aiMetricsRow.implicitHeight
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.maximumHeight: root.pageContentHeight

                    RowLayout {
                        id: aiMetricsRow
                        property real dualSharedStatusBarWidth: 24
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: root.aiDualColumnMode ? 8 : 0
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: root.aiTextVerticalOffset
                        spacing: root.aiEntryCount > 1 ? 10 : root.pageSpacing

                        function updateDualSharedStatusBarWidth() {
                            if (!root.aiDualColumnMode) {
                                dualSharedStatusBarWidth = 24
                                return
                            }

                            let maxWidth = 24
                            for (let index = 0; index < aiMetricsRepeater.count; ++index) {
                                const item = aiMetricsRepeater.itemAt(index)
                                if (item && item.headlineWidth !== undefined) {
                                    maxWidth = Math.max(maxWidth, item.headlineWidth)
                                }
                            }

                            dualSharedStatusBarWidth = maxWidth
                        }

                        Repeater {
                            id: aiMetricsRepeater
                            model: root.aiEntries

                            delegate: Item {
                                id: aiMetricDelegate
                                required property var modelData

                                readonly property string processLabel: modelData && modelData.processLabel ? String(modelData.processLabel) : "ai"
                                readonly property int runningCount: modelData && modelData.runningCount !== undefined ? Number(modelData.runningCount) : 0
                                readonly property int completedCount: modelData && modelData.completedCount !== undefined ? Number(modelData.completedCount) : 0
                                readonly property int totalCount: modelData && modelData.totalCount !== undefined ? Number(modelData.totalCount) : 0
                                readonly property bool running: runningCount > 0
                                readonly property bool completed: !running && totalCount > 0 && completedCount >= totalCount
                                readonly property string completedText: String(completedCount)
                                readonly property string totalText: String(totalCount)
                                readonly property int headlineSpacing: Math.max(2, Math.round(root.pageContentHeight * 0.06))
                                readonly property real headlineWidth: aiMetricHeadlineRow.implicitWidth

                                implicitWidth: aiMetricColumn.implicitWidth
                                implicitHeight: aiMetricColumn.implicitHeight
                                Layout.fillWidth: false
                                Layout.preferredWidth: implicitWidth
                                Layout.minimumWidth: 0

                                Component.onCompleted: aiMetricsRow.updateDualSharedStatusBarWidth()
                                onHeadlineWidthChanged: aiMetricsRow.updateDualSharedStatusBarWidth()

                                ColumnLayout {
                                    id: aiMetricColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: root.aiMetricColumnSpacing

                                    Item {
                                        id: aiMetricHeadlineRow
                                        implicitWidth: taskCountLabel.visible
                                            ? taskCountLabel.x + taskCountLabel.implicitWidth
                                            : totalTextItem.x + totalTextItem.implicitWidth
                                        implicitHeight: Math.max(completedTextItem.implicitHeight,
                                                                 taskCountLabel.visible ? taskCountLabel.implicitHeight : 0)

                                        Text {
                                            id: completedTextItem
                                            anchors.left: parent.left
                                            anchors.bottom: parent.bottom
                                            text: aiMetricDelegate.completedText
                                            color: root.primaryTextColor
                                            font.family: root.dataFontFamily
                                            font.pixelSize: root.aiPrimaryCountFontSize
                                            font.weight: Font.DemiBold
                                            renderType: Text.NativeRendering
                                        }

                                        Text {
                                            id: slashTextItem
                                            anchors.left: completedTextItem.right
                                            anchors.leftMargin: aiMetricDelegate.headlineSpacing
                                            anchors.bottom: completedTextItem.bottom
                                            text: "/"
                                            color: root.primaryTextColor
                                            font.family: root.dataFontFamily
                                            font.pixelSize: root.aiPrimaryCountFontSize
                                            font.weight: Font.DemiBold
                                            renderType: Text.NativeRendering
                                        }

                                        Text {
                                            id: totalTextItem
                                            anchors.left: slashTextItem.right
                                            anchors.leftMargin: aiMetricDelegate.headlineSpacing
                                            anchors.bottom: completedTextItem.bottom
                                            text: aiMetricDelegate.totalText
                                            color: root.primaryTextColor
                                            font.family: root.dataFontFamily
                                            font.pixelSize: root.aiSecondaryCountFontSize
                                            font.weight: Font.DemiBold
                                            renderType: Text.NativeRendering
                                        }

                                        Text {
                                            id: taskCountLabel
                                            anchors.left: totalTextItem.right
                                            anchors.leftMargin: aiMetricDelegate.headlineSpacing
                                            anchors.bottom: completedTextItem.bottom
                                            text: qsTr("条任务")
                                            color: root.secondaryTextColor
                                            font.pixelSize: root.captionFontSize
                                            font.weight: Font.Medium
                                            renderType: Text.NativeRendering
                                            visible: root.aiEntryCount === 1
                                        }
                                    }

                                    Rectangle {
                                        id: aiStatusTrack
                                        implicitWidth: root.aiDualColumnMode
                                            ? aiMetricsRow.dualSharedStatusBarWidth
                                            : aiMetricHeadlineRow.implicitWidth
                                        implicitHeight: root.aiStatusBarHeight
                                        radius: 0.1
                                        color: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.12)
                                        Layout.topMargin: root.aiStatusBarTopMargin

                                        Rectangle {
                                            anchors.fill: parent
                                            radius: parent.radius
                                            color: Qt.rgba(0.37, 0.83, 0.42, 0.95)
                                            visible: aiMetricDelegate.completed
                                        }

                                        Item {
                                            id: aiRunningViewport
                                            anchors.fill: parent
                                            clip: true
                                            visible: aiMetricDelegate.running

                                            Item {
                                                id: aiRunningMarquee
                                                width: aiStatusTrack.width * 2
                                                height: aiStatusTrack.height
                                                y: 0
                                                x: -aiStatusTrack.width

                                                Repeater {
                                                    model: 2

                                                    delegate: Rectangle {
                                                        x: index * aiStatusTrack.width
                                                        width: aiStatusTrack.width
                                                        height: aiStatusTrack.height
                                                        radius: 0.1
                                                        gradient: Gradient {
                                                            orientation: Gradient.Horizontal
                                                            GradientStop { position: 0.0; color: "#ff5f6d" }
                                                            GradientStop { position: 0.18; color: "#ff9966" }
                                                            GradientStop { position: 0.36; color: "#ffd84d" }
                                                            GradientStop { position: 0.54; color: "#59f2c1" }
                                                            GradientStop { position: 0.74; color: "#52a8ff" }
                                                            GradientStop { position: 1.0; color: "#d16cff" }
                                                        }
                                                    }
                                                }

                                                NumberAnimation on x {
                                                    running: aiMetricDelegate.running
                                                    loops: Animation.Infinite
                                                    from: -aiStatusTrack.width
                                                    to: 0
                                                    duration: root.aiStatusBarAnimationDuration
                                                    easing.type: Easing.Linear
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        text: aiMetricDelegate.processLabel
                                        color: root.secondaryTextColor
                                        font.pixelSize: root.aiProcessFontSize
                                        renderType: Text.NativeRendering
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        elide: Text.ElideRight
                                        visible: root.aiShowSecondaryLine
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        PageShell {
            id: musicPage
            pageId: "music"

            implicitWidth: root.musicPageVisible ? musicRow.implicitWidth : 0
            implicitHeight: root.musicPageVisible ? root.dockSize : 0
            onClicked: provider.openMusicPlayer()

            RowLayout {
                id: musicRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root.pluginRowLeftMargin
                anchors.rightMargin: root.rightContentPadding
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: root.musicPageVerticalOffset
                height: root.pageContentHeight
                spacing: root.musicRowSpacing

                Item {
                    implicitWidth: root.pluginLeadingSlotSize
                    implicitHeight: root.pluginLeadingSlotSize
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        id: musicArtworkBackground
                        anchors.centerIn: parent
                        width: root.musicArtSize
                        height: root.musicArtSize
                        radius: root.musicArtRadius
                        color: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.06)
                        visible: provider.musicHasArt && musicArtworkSource.status === Image.Ready
                    }

                    D.DciIcon {
                        anchors.centerIn: parent
                        width: root.musicArtSize
                        height: root.musicArtSize
                        name: provider.musicPlayerIconName
                        sourceSize: Qt.size(width, height)
                        retainWhileLoading: true
                        smooth: false
                        visible: !provider.musicHasArt || musicArtworkSource.status !== Image.Ready
                    }

                    Image {
                        id: musicArtworkSource
                        anchors.fill: musicArtworkBackground
                        visible: false
                        source: provider.musicArtSource
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: false
                        sourceSize: Qt.size(Math.round(root.musicArtSize * root.musicArtworkDevicePixelRatio), Math.round(root.musicArtSize * root.musicArtworkDevicePixelRatio))
                        mipmap: true
                        smooth: true
                    }

                    Rectangle {
                        id: musicArtworkMask
                        anchors.fill: musicArtworkBackground
                        radius: root.musicArtRadius
                        visible: false
                    }

                    OpacityMask {
                        anchors.fill: musicArtworkBackground
                        source: musicArtworkSource
                        maskSource: musicArtworkMask
                        visible: provider.musicHasArt && musicArtworkSource.status === Image.Ready
                        cached: false
                    }

                    Rectangle {
                        anchors.fill: musicArtworkBackground
                        radius: root.musicArtRadius
                        color: "transparent"
                        border.width: 1
                        border.color: Qt.rgba(0, 0, 0, 0.2)
                        visible: provider.musicHasArt && musicArtworkSource.status === Image.Ready
                    }
                }

                Item {
                    id: musicInfoArea
                    property int summaryWidth: Math.max(root.musicControlsWidth, Math.min(root.maxMusicTitleWidth, Math.ceil(Math.max(musicTitleItem.implicitWidth, (root.showSecondaryLine && musicSubtitleWrapper.visible) ? musicSubtitleItem.implicitWidth : 0))))
                    clip: true
                    implicitWidth: Math.ceil(summaryWidth + (root.musicControlsWidth - summaryWidth) * root.musicHoverProgress)
                    width: implicitWidth
                    implicitHeight: Math.max(musicInfoColumn.implicitHeight, musicControlsRow.implicitHeight)
                    Layout.alignment: Qt.AlignVCenter

                    Column {
                        id: musicInfoColumn
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        spacing: root.musicTextSpacing
                        opacity: 1 - root.musicHoverProgress
                        scale: 1 - root.musicHoverProgress * 0.08
                        x: -10 * root.musicHoverProgress
                        visible: opacity > 0.01

                        Item {
                            id: musicTitleWrapper
                            width: parent.width
                            implicitWidth: width
                            implicitHeight: musicTitleItem.implicitHeight
                            visible: provider.musicTitleText.length > 0

                            Text {
                                id: musicTitleItem
                                anchors.fill: parent
                                text: provider.musicTitleText
                                color: root.primaryTextColor
                                font.pixelSize: Math.max(11, root.headlineFontSize - 2)
                                font.weight: Font.Normal
                                renderType: Text.NativeRendering
                                horizontalAlignment: Text.AlignLeft
                                elide: Text.ElideRight
                            }
                        }

                        Item {
                            id: musicSubtitleWrapper
                            width: parent.width
                            implicitWidth: width
                            implicitHeight: musicSubtitleItem.implicitHeight
                            visible: root.showSecondaryLine && provider.musicSubtitleText.length > 0

                            Text {
                                id: musicSubtitleItem
                                anchors.fill: parent
                                text: provider.musicSubtitleText
                                color: root.secondaryTextColor
                                font.pixelSize: root.secondaryFontSize
                                renderType: Text.NativeRendering
                                horizontalAlignment: Text.AlignLeft
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Item {
                        id: musicControlsRow
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        implicitWidth: root.musicControlsWidth
                        implicitHeight: root.musicControlButtonSize
                        width: implicitWidth
                        height: implicitHeight
                        enabled: root.musicHoverProgress > 0.35

                        MusicActionButton {
                            layoutIndex: 0
                            hoverDelay: 24
                            iconName: "media-skip-backward"
                            actionEnabled: provider.musicCanGoPrevious
                            onClicked: provider.playPreviousTrack()
                        }

                        MusicActionButton {
                            layoutIndex: 1
                            hoverDelay: 0
                            iconName: provider.musicPlaying ? "media-playback-pause" : "media-playback-start"
                            actionEnabled: provider.musicCanTogglePlayback
                            onClicked: provider.toggleMusicPlayback()
                        }

                        MusicActionButton {
                            layoutIndex: 2
                            hoverDelay: 48
                            iconName: "media-skip-forward"
                            actionEnabled: provider.musicCanGoNext
                            onClicked: provider.playNextTrack()
                        }
                    }
                }
            }
        }

        PageShell {
            id: mailPage
            pageId: "mail"

            implicitWidth: mailRow.implicitWidth
            onClicked: {
                provider.openMailClient()
                root.completeMailAutoFocus()
            }

            RowLayout {
                id: mailRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: root.sharedPageVerticalOffset
                anchors.leftMargin: root.pluginRowLeftMargin
                anchors.rightMargin: root.rightContentPadding
                height: root.pageContentHeight
                spacing: root.pageSpacing

                Item {
                    implicitWidth: root.pluginLeadingSlotSize
                    implicitHeight: implicitWidth
                    Layout.alignment: Qt.AlignVCenter

                    D.DciIcon {
                        anchors.centerIn: parent
                        name: provider.mailIconName
                        width: root.pluginLeadingIconSize
                        height: root.pluginLeadingIconSize
                        sourceSize: Qt.size(width, height)
                        smooth: false
                    }
                }

                Item {
                    implicitWidth: mailTextColumn.implicitWidth
                    implicitHeight: mailTextColumn.implicitHeight
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.maximumHeight: root.pageContentHeight

                    ColumnLayout {
                        id: mailTextColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: root.sharedTextVerticalOffset
                        spacing: root.tightSpacing

                        RowLayout {
                            spacing: Math.max(3, Math.round(root.pageContentHeight * 0.1))

                            Text {
                                text: provider.mailUnreadCountText
                                color: root.primaryTextColor
                                font.family: root.dataFontFamily
                                font.pixelSize: root.headlineFontSize
                                font.weight: Font.DemiBold
                                renderType: Text.NativeRendering
                            }

                            Text {
                                text: qsTr("封未读")
                                color: root.secondaryTextColor
                                font.pixelSize: root.secondaryFontSize
                                font.weight: Font.Medium
                                renderType: Text.NativeRendering
                                Layout.alignment: root.showSecondaryLine ? Qt.AlignBottom : Qt.AlignVCenter
                            }
                        }

                        Text {
                            text: provider.mailSummaryText
                            color: root.secondaryTextColor
                            font.pixelSize: root.captionFontSize
                            renderType: Text.NativeRendering
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            visible: root.showSecondaryLine
                        }
                    }
                }
            }
        }

        PageShell {
            id: systemPage
            pageId: "system"

            implicitWidth: systemColumn.implicitWidth
            onClicked: provider.openSystemMonitorPage()

            ColumnLayout {
                id: systemColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: root.sharedPageVerticalOffset + root.sharedTextVerticalOffset
                anchors.leftMargin: root.leftContentPadding + root.foregroundContentOffset + root.systemForegroundContentOffset
                anchors.rightMargin: root.rightContentPadding
                height: root.pageContentHeight
                spacing: root.tightSpacing

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    spacing: root.pageSpacing

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: cpuMetricRow.implicitHeight

                        RowLayout {
                            id: cpuMetricRow
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Math.max(3, Math.round(root.pageContentHeight * 0.08))

                            Text {
                                width: systemMetricValueProbe.implicitWidth
                                text: qsTr("%1%").arg(provider.cpuUsage)
                                color: root.primaryTextColor
                                font.family: root.dataFontFamily
                                font.pixelSize: root.metricFontSize
                                font.weight: Font.DemiBold
                                renderType: Text.NativeRendering
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("CPU")
                                color: root.secondaryTextColor
                                font.pixelSize: root.captionFontSize
                                font.weight: Font.Medium
                                renderType: Text.NativeRendering
                                Layout.alignment: root.showSecondaryLine ? Qt.AlignBottom : Qt.AlignVCenter
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: memoryMetricRow.implicitHeight

                        RowLayout {
                            id: memoryMetricRow
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Math.max(3, Math.round(root.pageContentHeight * 0.08))

                            Text {
                                width: systemMetricValueProbe.implicitWidth
                                text: qsTr("%1%").arg(provider.memoryUsage)
                                color: root.primaryTextColor
                                font.family: root.dataFontFamily
                                font.pixelSize: root.metricFontSize
                                font.weight: Font.DemiBold
                                renderType: Text.NativeRendering
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("内存")
                                color: root.secondaryTextColor
                                font.pixelSize: root.captionFontSize
                                font.weight: Font.Medium
                                renderType: Text.NativeRendering
                                Layout.alignment: root.showSecondaryLine ? Qt.AlignBottom : Qt.AlignVCenter
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    spacing: root.pageSpacing
                    visible: root.showSecondaryLine

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: downloadSpeedTextItem.implicitHeight

                        Text {
                            id: downloadSpeedTextItem
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: systemTrafficProbe.implicitWidth
                            text: qsTr("%1 ↓").arg(provider.downloadSpeedText)
                            color: root.secondaryTextColor
                            font.family: root.dataFontFamily
                            font.pixelSize: root.monitorDetailFontSize
                            renderType: Text.NativeRendering
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: uploadSpeedTextItem.implicitHeight

                        Text {
                            id: uploadSpeedTextItem
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: systemTrafficProbe.implicitWidth
                            text: qsTr("%1 ↑").arg(provider.uploadSpeedText)
                            color: root.secondaryTextColor
                            font.family: root.dataFontFamily
                            font.pixelSize: root.monitorDetailFontSize
                            renderType: Text.NativeRendering
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
