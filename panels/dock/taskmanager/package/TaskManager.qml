// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml.Models 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.taskmanager 1.0
import org.deepin.dtk 1.0 as D

ContainmentItem {
    id: taskmanager
    property bool useColumnLayout: Panel.rootObject.useColumnLayout
    property int dockOrder: 16
    property real remainingSpacesForTaskManager: Panel.rootObject.adaptiveFashionMode ? 0 : (Panel.itemAlignment === Dock.LeftAlignment ? Panel.rootObject.dockLeftSpaceForCenter : Panel.rootObject.dockRemainingSpaceForCenter)
    readonly property bool centeredHorizontalFashionMode: Panel.viewMode === Dock.FashionMode
        && !Panel.rootObject.adaptiveFashionMode
        && !useColumnLayout
    readonly property bool resizeOptimizationActive: Panel.isResizing || (Panel.rootObject && Panel.rootObject.isDragging)
    readonly property int targetDockItemMaxSize: {
        if (Panel.rootObject.adaptiveFashionMode) {
            return Panel.rootObject.dockSize
        }

        const slotCount = Panel.rootObject.dockCenterPartCount - 1 + visualModel.count
        if (slotCount <= 0) {
            return Panel.rootObject.dockSize
        }

        return Math.min(Panel.rootObject.dockSize,
                        Panel.rootObject.dockLeftSpaceForCenter * 1.2 / slotCount - 2)
    }

    readonly property int appTitleSpacing: Math.max(10, Math.round(Panel.rootObject.dockItemMaxSize * 9 / 14) / 3)
    readonly property bool adaptiveFashionMinimumReached: Panel.rootObject.adaptiveFashionMode
        && !useColumnLayout
        && (Panel.rootObject.preferredDockSize <= Dock.MIN_DOCK_SIZE
            || Panel.rootObject.dockSize <= Dock.MIN_DOCK_SIZE)
    readonly property real adaptiveFashionOverflowHysteresis: 8
    readonly property real adaptiveFashionItemWidth: Math.max(1, Math.round(Panel.rootObject.dockItemMaxSize * 9 / 14 + appTitleSpacing))
    readonly property real adaptiveFashionOverflowButtonWidth: adaptiveFashionItemWidth
    readonly property real adaptiveFashionFullItemsWidth: {
        const totalCount = visualModel.items.count
        if (totalCount <= 0) {
            return 0
        }

        let width = 0
        for (let index = 0; index < totalCount; ++index) {
            if (index > 0) {
                width += appContainer.spacing
            }
            width += adaptiveFashionItemWidthForIndex(index)
        }
        return width
    }
    property bool adaptiveFashionOverflowLatched: false
    readonly property bool adaptiveFashionOverflowEnabled: Panel.rootObject.adaptiveFashionMode
        && !useColumnLayout
        && adaptiveFashionOverflowLatched
    property int adaptiveFashionVisibleItemCount: 0
    property var adaptiveFashionOverflowItems: []
    property var adaptiveFashionMeasuredItemWidths: ({})
    readonly property int adaptiveFashionOverflowCount: adaptiveFashionOverflowItems.length
    property bool adaptiveFashionOverflowSyncPending: false
    property real remainingSpacesForSplitWindow: Panel.rootObject.adaptiveFashionMode ? 0 : Math.max(0, Panel.rootObject.dockLeftSpaceForCenter - (
        (Panel.rootObject.dockCenterPartCount - 1) * (visualModel.cellWidth + appTitleSpacing) + (Panel.rootObject.dockCenterPartCount) * Panel.rootObject.dockPartSpacing))
    // 用于居中计算的实际应用区域尺寸
    property int appContainerWidth: useColumnLayout ? Panel.rootObject.dockSize : appContainer.implicitWidth
    property int appContainerHeight: useColumnLayout ? appContainer.implicitHeight : Panel.rootObject.dockSize
    property int appContainerTargetWidth: useColumnLayout ? Panel.rootObject.dockSize : appContainer.targetImplicitWidth
    property int appContainerTargetHeight: useColumnLayout ? appContainer.targetImplicitHeight : Panel.rootObject.dockSize
    
    implicitWidth: useColumnLayout
        ? Panel.rootObject.dockSize
        : (centeredHorizontalFashionMode ? appContainer.implicitWidth : Math.max(remainingSpacesForTaskManager, appContainer.implicitWidth))
    implicitHeight: useColumnLayout ? Math.max(remainingSpacesForTaskManager, appContainer.implicitHeight) : Panel.rootObject.dockSize

    // Helper function to find the current index of an app by its appId in the visualModel
    function findAppIndex(appId) {
        for (let i = 0; i < visualModel.items.count; i++) {
            const item = visualModel.items.get(i);
            if (item.model.itemId === appId) {
                return item.itemsIndex
            }
        }
        return -1
    }
    
    // Helper function to find the current index by window ID (for windowSplit mode)
    function findAppIndexByWindow(appId, winId) {
        if (!winId) return findAppIndex(appId)
        for (let i = 0; i < visualModel.items.count; i++) {
            const item = visualModel.items.get(i);
            if (item.model.itemId === appId && item.model.windows.length > 0 && item.model.windows[0] === winId) {
                return item.itemsIndex
            }
        }
        return -1
    }

    function findDockElementIndex(dockElement) {
        for (let i = 0; i < visualModel.items.count; i++) {
            const item = visualModel.items.get(i)
            if (item.model.dockElement === dockElement) {
                return item.itemsIndex
            }
        }
        return -1
    }

    function blendColorAlpha(fallback) {
        var appearance = DS.applet("org.deepin.ds.dde-appearance")
        if (!appearance || appearance.opacity < 0)
            return fallback
        return appearance.opacity
    }
    property real blendOpacity: blendColorAlpha(Panel.colorTheme === Dock.Dark ? 0.25 : 1.0)
    readonly property bool previewSwitchActive: previewSwitchGraceTimer.running

    function overflowItemDataAt(sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= visualModel.items.count) {
            return null
        }

        const item = visualModel.items.get(sourceIndex)
        if (!item || !item.model) {
            return null
        }

        const model = item.model
        return {
            active: model.active,
            attention: model.attention,
            itemId: model.itemId,
            dockElement: model.dockElement,
            itemKind: model.itemKind,
            name: model.name,
            title: model.title,
            iconName: model.iconName,
            previewIcons: model.previewIcons,
            menus: model.menus,
            windows: model.windows,
            visualIndex: sourceIndex,
            modelIndex: visualModel.modelIndex(sourceIndex)
        }
    }

    function scheduleAdaptiveFashionOverflowSync() {
        if (adaptiveFashionOverflowSyncPending) {
            return
        }

        adaptiveFashionOverflowSyncPending = true
        Qt.callLater(function() {
            adaptiveFashionOverflowSyncPending = false
            taskmanager.syncAdaptiveFashionOverflow()
        })
    }

    function adaptiveFashionWidthKeyAt(sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= visualModel.items.count) {
            return ""
        }

        const item = visualModel.items.get(sourceIndex)
        if (!item || !item.model) {
            return ""
        }

        const model = item.model
        if (model.dockElement && model.dockElement.length > 0) {
            return model.dockElement
        }

        if (model.itemId && model.itemId.length > 0) {
            return model.itemId
        }

        return String(sourceIndex)
    }

    function registerAdaptiveFashionItemWidth(key, width) {
        if (!key || !Number.isFinite(width) || width <= 0) {
            return
        }

        const currentWidth = adaptiveFashionMeasuredItemWidths[key]
        if (currentWidth === width) {
            return
        }

        adaptiveFashionMeasuredItemWidths = Object.assign({}, adaptiveFashionMeasuredItemWidths, {[key]: width})
        scheduleAdaptiveFashionOverflowSync()
    }

    function unregisterAdaptiveFashionItemWidth(key) {
        if (!key || adaptiveFashionMeasuredItemWidths[key] === undefined) {
            return
        }

        let nextWidths = Object.assign({}, adaptiveFashionMeasuredItemWidths)
        delete nextWidths[key]
        adaptiveFashionMeasuredItemWidths = nextWidths
        scheduleAdaptiveFashionOverflowSync()
    }

    function adaptiveFashionItemWidthForIndex(sourceIndex) {
        const widthKey = adaptiveFashionWidthKeyAt(sourceIndex)
        const measuredWidth = widthKey ? adaptiveFashionMeasuredItemWidths[widthKey] : undefined
        return (Number.isFinite(measuredWidth) && measuredWidth > 0) ? measuredWidth : adaptiveFashionItemWidth
    }

    function syncAdaptiveFashionOverflowEnabledState() {
        if (!Panel.rootObject.adaptiveFashionMode || useColumnLayout) {
            adaptiveFashionOverflowLatched = false
            return false
        }

        const totalCount = visualModel.items.count
        const availableWidth = Math.max(0, Panel.rootObject.adaptiveFashionAvailableTaskManagerWidth)
        if (totalCount <= 0 || !Number.isFinite(availableWidth) || availableWidth <= 0) {
            adaptiveFashionOverflowLatched = false
            return false
        }

        const startThreshold = availableWidth + adaptiveFashionOverflowHysteresis
        const stopThreshold = Math.max(0, availableWidth - adaptiveFashionOverflowHysteresis)
        const totalItemsWidth = adaptiveFashionFullItemsWidth
        const nextLatched = adaptiveFashionOverflowLatched
            ? (totalItemsWidth > stopThreshold)
            : (adaptiveFashionMinimumReached && totalItemsWidth > startThreshold)

        if (adaptiveFashionOverflowLatched !== nextLatched) {
            adaptiveFashionOverflowLatched = nextLatched
        }

        return nextLatched
    }

    function syncAdaptiveFashionOverflow() {
        const totalCount = visualModel.items.count
        const overflowEnabled = syncAdaptiveFashionOverflowEnabledState()
        if (!overflowEnabled || totalCount <= 0) {
            adaptiveFashionVisibleItemCount = totalCount
            adaptiveFashionOverflowItems = []
            return
        }

        const availableWidth = Math.max(0, Panel.rootObject.adaptiveFashionAvailableTaskManagerWidth)
        if (!Number.isFinite(availableWidth) || availableWidth <= 0) {
            adaptiveFashionVisibleItemCount = totalCount
            adaptiveFashionOverflowItems = []
            return
        }

        const overflowWidth = adaptiveFashionOverflowButtonWidth
        const spacing = appContainer.spacing

        let usedWidth = 0
        let visibleCount = 0
        for (let index = 0; index < totalCount; ++index) {
            const itemWidth = adaptiveFashionItemWidthForIndex(index)
            const leadingSpacing = visibleCount > 0 ? spacing : 0
            const remainingCount = totalCount - (visibleCount + 1)
            const reservedOverflowWidth = remainingCount > 0 ? spacing + overflowWidth : 0
            if (usedWidth + leadingSpacing + itemWidth + reservedOverflowWidth > availableWidth) {
                break
            }

            usedWidth += leadingSpacing + itemWidth
            visibleCount++
        }

        if (visibleCount >= totalCount) {
            adaptiveFashionVisibleItemCount = totalCount
            adaptiveFashionOverflowItems = []
            return
        }

        let overflowItems = []
        for (let index = visibleCount; index < totalCount; ++index) {
            const itemData = overflowItemDataAt(index)
            if (itemData) {
                overflowItems.push(itemData)
            }
        }

        adaptiveFashionVisibleItemCount = visibleCount
        adaptiveFashionOverflowItems = overflowItems
    }

    function beginPreviewSwitch() {
        previewSwitchGraceTimer.restart()
    }

    function endPreviewSwitch() {
        previewSwitchGraceTimer.stop()
    }

    function applyDockItemMaxSize() {
        if (Panel.rootObject.dockItemMaxSize !== targetDockItemMaxSize) {
            Panel.rootObject.dockItemMaxSize = targetDockItemMaxSize
        }
    }

    onTargetDockItemMaxSizeChanged: {
        if (resizeOptimizationActive) {
            dockItemSizeSyncTimer.restart()
            return
        }

        applyDockItemMaxSize()
    }

    onResizeOptimizationActiveChanged: {
        if (resizeOptimizationActive) {
            return
        }

        dockItemSizeSyncTimer.stop()
        applyDockItemMaxSize()
    }

    onAdaptiveFashionOverflowEnabledChanged: scheduleAdaptiveFashionOverflowSync()
    onAdaptiveFashionMinimumReachedChanged: scheduleAdaptiveFashionOverflowSync()
    onAdaptiveFashionFullItemsWidthChanged: scheduleAdaptiveFashionOverflowSync()
    onAdaptiveFashionItemWidthChanged: scheduleAdaptiveFashionOverflowSync()
    onUseColumnLayoutChanged: scheduleAdaptiveFashionOverflowSync()

    TextCalculator {
        id: textCalculator
        enabled: taskmanager.Applet.windowSplit && (Panel.position == Dock.Bottom || Panel.position == Dock.Top)
        dataModel: taskmanager.Applet.dataModel
        iconSize: Panel.rootObject.dockItemMaxSize * 9 / 14
        spacing: appContainer.spacing
        cellSize: visualModel.cellWidth
        itemPadding: 4
        remainingSpace: taskmanager.remainingSpacesForSplitWindow
        font.family: D.DTK.fontManager.t6.family
        font.pixelSize: Math.max(10, Math.min(20, Math.round(textCalculator.iconSize * 0.35)))
    }

    Timer {
        id: previewSwitchGraceTimer
        interval: 96
        repeat: false
    }

    Timer {
        id: dockItemSizeSyncTimer
        interval: 16
        repeat: false
        onTriggered: {
            taskmanager.applyDockItemMaxSize()
        }
    }

    Connections {
        target: Panel.rootObject

        function onAdaptiveFashionAvailableTaskManagerWidthChanged() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onDockItemMaxSizeChanged() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onPreferredDockSizeChanged() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }
    }

    Connections {
        target: taskmanager.Applet.dataModel

        function onDataChanged() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onRowsInserted() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onRowsMoved() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onRowsRemoved() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onModelReset() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }

        function onLayoutChanged() {
            taskmanager.scheduleAdaptiveFashionOverflowSync()
        }
    }

    Component {
        id: overflowFooterComponent

        Item {
            id: overflowFooter
            readonly property int previewCount: Math.min(taskmanager.adaptiveFashionOverflowItems.length, 3)
            readonly property bool singleOverflowItem: taskmanager.adaptiveFashionOverflowCount === 1
            readonly property int previewStartIndex: Math.max(0, taskmanager.adaptiveFashionOverflowItems.length - previewCount)
            readonly property var primaryOverflowItem: taskmanager.adaptiveFashionOverflowItems.length > 0
                ? taskmanager.adaptiveFashionOverflowItems[0]
                : null
            readonly property real stackedIconSize: Math.max(12, Math.round(Panel.rootObject.dockItemIconSize * 0.78))
            readonly property real stackXOffset: Math.max(2, Math.round(stackedIconSize * 0.24))
            readonly property real hoverHeight: Math.round(Panel.rootObject.dockItemIconSize + 8)
            readonly property int popupColumns: Math.min(6, Math.max(1, taskmanager.adaptiveFashionOverflowCount))
            readonly property int popupInnerMargin: 10
            property point popupAnchorPoint: Qt.point(0, 0)

            implicitWidth: taskmanager.adaptiveFashionOverflowButtonWidth
            implicitHeight: taskmanager.implicitHeight
            width: implicitWidth
            height: implicitHeight

            function togglePopup() {
                if (taskmanager.adaptiveFashionOverflowCount <= 0) {
                    return
                }

                if (overflowPopup.popupVisible) {
                    overflowPopup.close()
                    return
                }

                Panel.requestClosePopup()
                popupAnchorPoint = overflowFooter.mapToItem(null, overflowFooter.width / 2, overflowFooter.height / 2)
                overflowPopup.open()
            }

            AppletItemBackground {
                width: overflowFooter.width
                height: overflowFooter.hoverHeight
                anchors.centerIn: parent
                enabled: false
                opacity: overflowMouseArea.containsMouse
                    || overflowPopup.popupVisible
                    || (overflowFooter.singleOverflowItem
                        && overflowFooter.primaryOverflowItem
                        && overflowFooter.primaryOverflowItem.active
                        && overflowFooter.primaryOverflowItem.windows.length > 0)
                    ? 1.0
                    : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 150 }
                }
            }

            AppItem {
                anchors.fill: parent
                visible: overflowFooter.singleOverflowItem && overflowFooter.primaryOverflowItem
                enabled: false
                displayMode: Panel.indicatorStyle
                colorTheme: Panel.colorTheme
                itemActive: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.active : false
                attention: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.attention : false
                itemId: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.itemId : ""
                dockElement: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.dockElement : ""
                itemKind: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.itemKind : ""
                name: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.name : ""
                iconName: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.iconName : ""
                previewIcons: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.previewIcons : []
                menus: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.menus : ""
                windows: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.windows : []
                visualIndex: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.visualIndex : -1
                modelIndex: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.modelIndex : null
                blendOpacity: taskmanager.blendOpacity
                title: overflowFooter.primaryOverflowItem ? overflowFooter.primaryOverflowItem.title : ""
                enableTitle: false
                appTitleSpacing: taskmanager.appTitleSpacing
            }

            Item {
                anchors.centerIn: parent
                visible: !overflowFooter.singleOverflowItem
                width: overflowFooter.stackedIconSize
                    + overflowFooter.stackXOffset * Math.max(0, overflowFooter.previewCount - 1)
                height: overflowFooter.stackedIconSize

                Repeater {
                    model: overflowFooter.previewCount

                    D.DciIcon {
                        required property int index

                        readonly property int previewIndex: overflowFooter.previewStartIndex + index
                        readonly property var previewItem: taskmanager.adaptiveFashionOverflowItems[previewIndex]

                        name: previewItem && previewItem.iconName ? previewItem.iconName : ""
                        width: overflowFooter.stackedIconSize
                        height: overflowFooter.stackedIconSize
                        sourceSize: Qt.size(width, height)
                        smooth: false
                        retainWhileLoading: true
                        x: index * overflowFooter.stackXOffset
                        y: 0
                        opacity: 0.7 + index * 0.15
                    }
                }
            }

            MouseArea {
                id: overflowMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                onClicked: overflowFooter.togglePopup()
            }

            PanelPopup {
                id: overflowPopup
                width: overflowPopupContent.width
                height: overflowPopupContent.height
                popupX: {
                    switch (Panel.position) {
                    case Dock.Top:
                    case Dock.Bottom:
                        return overflowFooter.popupAnchorPoint.x - overflowPopup.width / 2
                    case Dock.Right:
                        return -overflowPopup.width - overflowFooter.popupInnerMargin
                    case Dock.Left:
                        return Panel.rootObject.dockSize + overflowFooter.popupInnerMargin
                    }

                    return overflowFooter.popupAnchorPoint.x - overflowPopup.width / 2
                }
                popupY: {
                    switch (Panel.position) {
                    case Dock.Top:
                        return Panel.rootObject.dockSize + overflowFooter.popupInnerMargin
                    case Dock.Right:
                    case Dock.Left:
                        return overflowFooter.popupAnchorPoint.y - overflowPopup.height / 2
                    case Dock.Bottom:
                        return -overflowPopup.height - overflowFooter.popupInnerMargin
                    }

                    return -overflowPopup.height - overflowFooter.popupInnerMargin
                }

                Control {
                    id: overflowPopupContent
                    padding: 10
                    implicitWidth: overflowGrid.implicitWidth + leftPadding + rightPadding
                    implicitHeight: overflowGrid.implicitHeight + topPadding + bottomPadding
                    width: implicitWidth
                    height: implicitHeight

                    background: Item {}

                    contentItem: Grid {
                        id: overflowGrid
                        columns: overflowFooter.popupColumns
                        rowSpacing: Math.max(6, Math.round(taskmanager.adaptiveFashionOverflowButtonWidth * 0.12))
                        columnSpacing: rowSpacing

                        Repeater {
                            model: taskmanager.adaptiveFashionOverflowItems

                            delegate: Item {
                                required property var modelData

                                width: taskmanager.adaptiveFashionOverflowButtonWidth
                                height: taskmanager.implicitHeight

                                AppItem {
                                    anchors.fill: parent
                                    displayMode: Panel.indicatorStyle
                                    colorTheme: Panel.colorTheme
                                    itemActive: modelData.active
                                    attention: modelData.attention
                                    itemId: modelData.itemId
                                    dockElement: modelData.dockElement
                                    itemKind: modelData.itemKind
                                    name: modelData.name
                                    iconName: modelData.iconName
                                    previewIcons: modelData.previewIcons
                                    menus: modelData.menus
                                    windows: modelData.windows
                                    visualIndex: modelData.visualIndex
                                    modelIndex: modelData.modelIndex
                                    blendOpacity: taskmanager.blendOpacity
                                    title: modelData.title
                                    enableTitle: false
                                    appTitleSpacing: taskmanager.appTitleSpacing

                                    Component.onCompleted: {
                                        dropFilesOnItem.connect(taskmanager.Applet.dropFilesOnItem)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        useColumnLayout: taskmanager.useColumnLayout
        spacing: 0
        remove: Transition {
            NumberAnimation {
                properties: "scale,opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
        model: DelegateModel {
            id: visualModel
            model: taskmanager.Applet.dataModel
            // 1:4 the distance between app : dock height; get width/height≈0.8
            property real cellWidth: Panel.rootObject.dockItemMaxSize * 0.8
            delegate: Item {
                id: delegateRoot
                required property int index
                required property bool active
                required property bool attention
                required property string itemId
                required property string dockElement
                required property string itemKind
                required property string name
                required property string title // winTitle
                required property string iconName
                required property string icon // winIconName
                required property var previewIcons
                required property string menus
                required property list<string> windows
                z: attention ? -1 : 0
                readonly property bool overflowProxyItem: taskmanager.adaptiveFashionOverflowEnabled
                    && taskmanager.adaptiveFashionOverflowCount > 0
                    && DelegateModel.itemsIndex === taskmanager.adaptiveFashionVisibleItemCount
                readonly property bool hiddenByOverflow: taskmanager.adaptiveFashionOverflowEnabled
                    && DelegateModel.itemsIndex > taskmanager.adaptiveFashionVisibleItemCount
                readonly property bool hiddenByDrag: {
                    let draggedAppId = taskmanager.Applet.desktopIdToAppId(launcherDndDropArea.launcherDndDesktopId)
                    if (itemId !== draggedAppId) {
                        return false
                    }

                    if (windows.length === 0) {
                        return true
                    }

                    return launcherDndDropArea.launcherDndWinId !== windows[0]
                }
                property bool visibility: !hiddenByOverflow && !hiddenByDrag
                readonly property real layoutImplicitWidth: useColumnLayout ? taskmanager.implicitWidth : appItem.implicitWidth
                readonly property real layoutImplicitHeight: useColumnLayout ? visualModel.cellWidth : taskmanager.implicitHeight
                readonly property real targetImplicitWidth: hiddenByOverflow ? 0 : layoutImplicitWidth
                readonly property real targetImplicitHeight: hiddenByOverflow ? 0 : layoutImplicitHeight
                readonly property string adaptiveFashionWidthKey: dockElement && dockElement.length > 0
                    ? dockElement
                    : (itemId && itemId.length > 0 ? itemId : String(DelegateModel.itemsIndex))

                ListView.onAdd: NumberAnimation {
                    target: delegateRoot
                    properties: "scale,opacity"
                    from: 0
                    to: 1
                    duration: 200
                }

                states: [
                    State {
                        name: "item-visible"
                        when: delegateRoot.visibility
                        PropertyChanges { target: delegateRoot; opacity: 1.0; scale: 1.0; }
                    },
                    State {
                        name: "item-invisible"
                        when: !delegateRoot.visibility
                        PropertyChanges { target: delegateRoot; opacity: 0.0; scale: 0.0; }
                    }
                ]

                Behavior on opacity { NumberAnimation { duration: 200 } }
                Behavior on scale { NumberAnimation { duration: 200 } }

                implicitWidth: targetImplicitWidth
                implicitHeight: targetImplicitHeight
                width: targetImplicitWidth
                height: targetImplicitHeight
                visible: !hiddenByOverflow

                function syncAdaptiveFashionMeasuredWidth() {
                    taskmanager.registerAdaptiveFashionItemWidth(adaptiveFashionWidthKey, layoutImplicitWidth)
                }

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

                onLayoutImplicitWidthChanged: syncAdaptiveFashionMeasuredWidth()
                onAdaptiveFashionWidthKeyChanged: syncAdaptiveFashionMeasuredWidth()

                Component.onCompleted: {
                    syncAdaptiveFashionMeasuredWidth()
                }

                Component.onDestruction: {
                    taskmanager.unregisterAdaptiveFashionItemWidth(adaptiveFashionWidthKey)
                }

                Rectangle {
                    // kept for debug purpose
                    // border.color: "red"
                    // border.width: 1
                    id: appItemRect
                    color: "transparent"
                    visible: !delegateRoot.overflowProxyItem && !delegateRoot.hiddenByOverflow
                    parent: appContainer
                    x: delegateRoot.x
                    y: delegateRoot.y
                    width: delegateRoot.width
                    height: delegateRoot.height
                    scale: delegateRoot.scale
                    property bool positionAnimationEnabled: false
                    Behavior on x {
                        enabled: appItemRect.positionAnimationEnabled
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on y {
                        enabled: appItemRect.positionAnimationEnabled
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.OutCubic
                        }
                    }

                    Component.onCompleted: {
                        Qt.callLater(function() {
                            appItemRect.positionAnimationEnabled = true
                        })
                    }

                    AppItem {
                        id: appItem
                        anchors.fill: parent // This is mandatory for draggable item center in drop area

                        displayMode: Panel.indicatorStyle
                        colorTheme: Panel.colorTheme
                        itemActive: delegateRoot.active
                        attention: delegateRoot.attention
                        itemId: delegateRoot.itemId
                        dockElement: delegateRoot.dockElement
                        itemKind: delegateRoot.itemKind
                        name: delegateRoot.name
                        iconName: delegateRoot.iconName
                        previewIcons: delegateRoot.previewIcons
                        menus: delegateRoot.menus
                        windows: delegateRoot.windows
                        visualIndex: delegateRoot.visualIndex
                        modelIndex: delegateRoot.modelIndex
                        blendOpacity: taskmanager.blendOpacity
                        title: delegateRoot.title
                        enableTitle: textCalculator.enabled
                        appTitleSpacing: taskmanager.appTitleSpacing
                        ListView.delayRemove: Drag.active
                        Component.onCompleted: {
                            dropFilesOnItem.connect(taskmanager.Applet.dropFilesOnItem)
                        }
                        onDragFinished: function() {
                            launcherDndDropArea.resetDndState()
                        }
                    }
                }

                Loader {
                    id: overflowProxyLoader
                    active: delegateRoot.overflowProxyItem
                    visible: delegateRoot.overflowProxyItem
                    parent: appContainer
                    x: delegateRoot.x
                    y: delegateRoot.y
                    width: delegateRoot.width
                    height: delegateRoot.height
                    scale: delegateRoot.scale
                    opacity: delegateRoot.opacity
                    sourceComponent: overflowFooterComponent
                }
            }
        }

        DropArea {
            id: launcherDndDropArea
            anchors.fill: parent
            z: 3
            property string launcherDndDesktopId: ""
            property string launcherDndDragSource: ""
            property string launcherDndWinId: ""
            property string pendingDockElement: ""
            property string pendingFolderUrl: ""

            function resetDndState() {
                launcherDndDesktopId = ""
                launcherDndDragSource = ""
                launcherDndWinId = ""
                pendingDockElement = ""
                pendingFolderUrl = ""
            }

            function dragString(drag, key) {
                if (!drag || drag.getDataAsString === undefined || drag.getDataAsString === null) {
                    return ""
                }

                const value = drag.getDataAsString(key)
                if (value === undefined || value === null) {
                    return ""
                }
                return String(value)
            }

            function urlsFromText(rawText) {
                const text = rawText ? String(rawText).trim() : ""
                if (text === "") {
                    return []
                }

                return text.split(/[\r\n]+/).filter(function(entry) {
                    return entry !== ""
                })
            }

            function dragUrls(drag) {
                const urls = drag.urls || []
                if (urls.length > 0) {
                    let result = []
                    for (let i = 0; i < urls.length; ++i) {
                        result.push(String(urls[i]))
                    }
                    return result
                }

                const treeUrls = urlsFromText(dragString(drag, "dfm_tree_urls_for_drag"))
                if (treeUrls.length > 0) {
                    return treeUrls
                }

                return urlsFromText(dragString(drag, "text/uri-list"))
            }

            function launcherDesktopIdFromDrag(drag) {
                let desktopId = dragString(drag, "text/x-dde-dock-dnd-appid")
                if (desktopId !== "") {
                    return desktopId
                }

                desktopId = dragString(drag, "text/x-dde-launcher-dnd-desktopId")
                if (desktopId !== "") {
                    return desktopId
                }

                if (!drag.source) {
                    return ""
                }

                if (drag.source.desktopId !== undefined && drag.source.desktopId !== null && drag.source.desktopId !== "") {
                    return String(drag.source.desktopId)
                }

                if (drag.source.itemId !== undefined && drag.source.itemId !== null && drag.source.itemId !== "") {
                    return String(drag.source.itemId)
                }

                if (drag.source.appId !== undefined && drag.source.appId !== null && drag.source.appId !== "") {
                    return String(drag.source.appId)
                }

                return ""
            }

            function candidateFolderUrl(drag) {
                const urls = dragUrls(drag)
                if (urls.length !== 1) {
                    return ""
                }

                const candidate = urls[0]
                if (candidate.indexOf("file://") !== 0) {
                    return ""
                }

                return candidate
            }

            function currentDragIndex() {
                if (pendingDockElement === "") {
                    return -1
                }

                if (taskmanager.Applet.windowSplit && pendingDockElement.indexOf("desktop/") === 0) {
                    let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                    if (launcherDndDragSource === "taskbar" && launcherDndWinId !== "") {
                        return taskmanager.findAppIndexByWindow(appId, launcherDndWinId)
                    }
                    return taskmanager.findAppIndex(appId)
                }

                return taskmanager.findDockElementIndex(pendingDockElement)
            }

            function logDrag(prefix, drag, extra) {
                console.warn(prefix,
                             "source=", launcherDndDragSource,
                             "desktopId=", launcherDndDesktopId,
                             "dockElement=", pendingDockElement,
                             "folderUrl=", pendingFolderUrl,
                             "dockAppId=", dragString(drag, "text/x-dde-dock-dnd-appid"),
                             "launcherDesktopId=", dragString(drag, "text/x-dde-launcher-dnd-desktopId"),
                             "appType=", dragString(drag, "dfm_app_type_for_drag"),
                             "treeUrls=", dragString(drag, "dfm_tree_urls_for_drag"),
                             "uriList=", dragString(drag, "text/uri-list"),
                             "urls=", JSON.stringify(dragUrls(drag)),
                             extra || "")
            }

            onEntered: function(drag) {
                launcherDndDragSource = dragString(drag, "text/x-dde-dock-dnd-source")
                launcherDndWinId = dragString(drag, "text/x-dde-dock-dnd-winid")
                launcherDndDesktopId = launcherDesktopIdFromDrag(drag)
                pendingDockElement = dragString(drag, "text/x-dde-dock-dnd-element")
                pendingFolderUrl = ""

                if (launcherDndDragSource === "" && launcherDndDesktopId !== "") {
                    launcherDndDragSource = "launcher"
                }

                logDrag("taskmanager drag entered", drag)

                if (launcherDndDragSource === "taskbar") {
                    if (pendingDockElement === "" && launcherDndDesktopId !== "") {
                        pendingDockElement = taskmanager.Applet.dockElementFromLauncherId(launcherDndDesktopId)
                    }
                    if (pendingDockElement === "") {
                        drag.accepted = false
                        resetDndState()
                    } else {
                        drag.accepted = true
                    }
                    return
                }

                if (launcherDndDesktopId !== "") {
                    pendingDockElement = taskmanager.Applet.dockElementFromLauncherId(launcherDndDesktopId)
                    if (pendingDockElement === "" || taskmanager.Applet.requestDockByDesktopId(launcherDndDesktopId) === false) {
                        logDrag("taskmanager launcher drag rejected", drag)
                        drag.accepted = false
                        resetDndState()
                    } else {
                        drag.accepted = true
                    }
                    return
                }

                const folderUrl = candidateFolderUrl(drag)
                if (folderUrl !== "") {
                    pendingFolderUrl = folderUrl
                    pendingDockElement = taskmanager.Applet.folderUrlToElementId(pendingFolderUrl)
                    if (pendingDockElement === "" || taskmanager.Applet.requestDockByFolderUrl(pendingFolderUrl) === false) {
                        logDrag("taskmanager folder drag rejected", drag)
                        drag.accepted = false
                        resetDndState()
                    } else {
                        drag.accepted = true
                    }
                    return
                }

                logDrag("taskmanager drag unsupported", drag)
                drag.accepted = false
                resetDndState()
            }

            onPositionChanged: function(drag) {
                if (pendingDockElement === "") return
                let targetIndex = appContainer.indexAt(drag.x, drag.y)
                let currentIndex = currentDragIndex()
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    if (taskmanager.Applet.windowSplit) {
                        taskmanager.Applet.moveItem(currentIndex, targetIndex)
                    } else {
                        visualModel.items.move(currentIndex, targetIndex)
                        taskmanager.scheduleAdaptiveFashionOverflowSync()
                    }
                }
            }

            onDropped: function(drop) {
                logDrag("taskmanager drag dropped", drop)
                Panel.contextDragging = false
                if (pendingDockElement === "") return
                drop.accepted = true
                let targetIndex = appContainer.indexAt(drop.x, drop.y)
                let currentIndex = currentDragIndex()
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    if (taskmanager.Applet.windowSplit) {
                        taskmanager.Applet.moveItem(currentIndex, targetIndex)
                    } else {
                        visualModel.items.move(currentIndex, targetIndex)
                        taskmanager.scheduleAdaptiveFashionOverflowSync()
                    }
                }
                let dockElements = []
                for (let i = 0; i < visualModel.items.count; i++) {
                    dockElements.push(visualModel.items.get(i).model.dockElement)
                }
                taskmanager.Applet.saveDockElementsOrder(dockElements)
                resetDndState()
            }

            onExited: function(drag) {
                logDrag("taskmanager drag exited", drag)
                if (launcherDndDesktopId !== "" && launcherDndDragSource !== "taskbar") {
                    taskmanager.Applet.requestUndockByDesktopId(launcherDndDesktopId)
                }
                if (pendingFolderUrl !== "" && launcherDndDragSource !== "taskbar") {
                    taskmanager.Applet.requestUndockByFolderUrl(pendingFolderUrl)
                }
                resetDndState()
            }
        }
    }

    Component.onCompleted: {
        applyDockItemMaxSize()
        scheduleAdaptiveFashionOverflowSync()
    }
}
