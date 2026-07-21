// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.taskmanager 1.0
import org.deepin.dtk 1.0 as D

ContainmentItem {
    id: taskmanager
    property bool useColumnLayout: Panel.rootObject.useColumnLayout
    property int dockOrder: 16
    readonly property bool fashionMode: Panel.fashionMode

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Task Manager")

    function calcRemainingSpace(baseSize) {
        const otherCount = Panel.rootObject.dockCenterPartCount - 1;
        const otherOccupied = otherCount > 0 ? otherCount * baseSize * multitaskViewIconRatio : 0;
        return Panel.rootObject.dockEffectiveCenterSpace - otherOccupied;
    }

    property real remainingSpacesForTaskManager: fashionMode
        ? 0
        : calcRemainingSpace(Panel.rootObject.dockItemMaxSize)
    readonly property int appTitleSpacing: Math.max(10, Math.round(Panel.rootObject.dockItemMaxSize * 9 / 14) / 3)
    // Start padding for the app container so that the visual gap
    // (multitask icon right edge → first app icon left edge) = appTitleSpacing.
    // Compensates for the multitask view's box being wider than its icon.
    // 4/5 = 0.8: multitask button width relative to dockItemMaxSize.
    // Derived from the 1:4 spacing ratio (app icon : dock item height = 4:1, so content = 4/5 = 0.8).
    // Matches AppletDockItem.qml implicitWidth/Height ratio.
    readonly property real multitaskViewIconRatio: 0.8
    // 9/14 ≈ 0.64: app icon size relative to dockItemMaxSize (iconSize:dockHeight = 9:14).
    // Defined in main.qml as dockItemIconSize = dockItemMaxSize * 9 / 14.
    // At default dock size (56), icon = 36px; at min (37), icon ≈ 24px; at max (100), icon ≈ 64px.
    readonly property real iconWidthToMaxSizeRatio: 9 / 14
    readonly property int minimumDockItemSize: Math.max(
        Dock.MIN_DOCK_SIZE,
        Math.round(Panel.rootObject.dockSize * Dock.MIN_DOCK_SIZE / Dock.DEFAULT_DOCK_SIZE))
    readonly property real startPadding: Math.max(0, appTitleSpacing - (Panel.rootObject.dockItemMaxSize * (multitaskViewIconRatio - iconWidthToMaxSizeRatio) / 2))

    implicitWidth: {
        if (fashionMode) return appContainer.implicitWidth + (useColumnLayout ? 0 : startPadding)
        let extra = useColumnLayout ? 0 : startPadding
        let w = appContainer.implicitWidth + extra
        let maxW = Panel.itemAlignment === Dock.LeftAlignment ? Math.max(remainingSpacesForTaskManager, w) : Math.min(remainingSpacesForTaskManager, w)
        return useColumnLayout ? Panel.rootObject.dockSize : maxW
    }
    implicitHeight: {
        if (fashionMode && useColumnLayout) return appContainer.implicitHeight + startPadding
        let extra = useColumnLayout ? startPadding : 0
        let h = appContainer.implicitHeight + extra
        let maxH = Panel.itemAlignment === Dock.LeftAlignment ? Math.max(remainingSpacesForTaskManager, h) : Math.min(remainingSpacesForTaskManager, h)
        return useColumnLayout ? maxH : Panel.rootObject.dockSize
    }
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

    function blendColorAlpha(fallback) {
        var appearance = DS.applet("org.deepin.ds.dde-appearance")
        if (!appearance || appearance.opacity < 0)
            return fallback
        return appearance.opacity
    }
    property real blendOpacity: blendColorAlpha(D.DTK.themeType === D.ApplicationHelper.DarkType ? 0.25 : 1.0)

    TaskOverflowController {
        id: taskOverflow
        // Window-split title layout has priority. Only fall back to adaptive icon
        // shrinking and overflow when no title layout can fit the available space.
        enabled: !textCalculator.enabled || textCalculator.totalWidth <= 0
        availableExtent: Math.max(0, taskmanager.calcRemainingSpace(Panel.rootObject.dockItemMaxSize) - taskmanager.startPadding)
        minimumReached: Panel.rootObject.dockItemMaxSize <= taskmanager.minimumDockItemSize + 0.5
        fallbackItemExtent: Math.max(1, Panel.rootObject.dockItemMaxSize * taskmanager.iconWidthToMaxSizeRatio)
        spacing: taskmanager.appTitleSpacing
        visualModel: visualModel
        dataModel: taskmanager.Applet.dataModel
    }

    TextCalculator {
        id: textCalculator
        enabled: !fashionMode && taskmanager.Applet.windowSplit && (Panel.position == Dock.Bottom || Panel.position == Dock.Top)
        dataModel: taskmanager.Applet.dataModel
        iconSize: Panel.rootObject.dockSize * 9 / 14
        spacing: Math.max(10, Math.round(textCalculator.iconSize) / 3)
        cellSize: textCalculator.iconSize
        itemPadding: Math.round(textCalculator.iconSize / 8)
        remainingSpace: {
            const dockIconSize = Panel.rootObject.dockSize;
            const startPadding = Math.max(0, textCalculator.spacing - (dockIconSize * (multitaskViewIconRatio - iconWidthToMaxSizeRatio) / 2));
            return calcRemainingSpace(dockIconSize) - startPadding;
        }
        font.family: D.DTK.fontManager.t6.family
        font.pixelSize: Math.max(10, Math.min(20, Math.round(textCalculator.iconSize * 0.35)))
    }

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        anchors.leftMargin: useColumnLayout ? 0 : taskmanager.startPadding
        anchors.topMargin: useColumnLayout ? taskmanager.startPadding : 0
        useColumnLayout: taskmanager.useColumnLayout
        spacing: taskmanager.appTitleSpacing
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
                required property string name
                required property string title // winTitle
                required property string iconName
                required property string icon // winIconName
                required property string menus
                required property list<string> windows
                z: attention ? -1 : 0
                property bool visibility: {
                    let draggedAppId = taskmanager.Applet.desktopIdToAppId(launcherDndDropArea.launcherDndDesktopId)
                    if (itemId !== draggedAppId) {
                        return true 
                    }
                    return windows.length > 0 && launcherDndDropArea.launcherDndWinId !== windows[0]
                }

                readonly property bool overflowActive: taskOverflow.popupItems.length > 0
                readonly property bool isOverflowButton: overflowActive
                    && DelegateModel.itemsIndex === taskOverflow.visibleItemCount
                readonly property bool hiddenInOverflow: overflowActive
                    && DelegateModel.itemsIndex > taskOverflow.visibleItemCount
                visible: !hiddenInOverflow

                readonly property real normalDelegateWidth: useColumnLayout
                    ? taskmanager.implicitWidth
                    : appItem.implicitWidth
                readonly property real normalDelegateHeight: useColumnLayout
                    ? Panel.rootObject.dockItemMaxSize * 9 / 14
                    : taskmanager.implicitHeight

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

                implicitWidth: hiddenInOverflow? 0 : normalDelegateWidth
                implicitHeight: hiddenInOverflow? 0 : normalDelegateHeight

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

                Rectangle {
                    // kept for debug purpose
                    // border.color: "red"
                    // border.width: 1
                    id: appItemRect
                    color: "transparent"
                    parent: appContainer
                    x: delegateRoot.x
                    y: delegateRoot.y
                    width: delegateRoot.width
                    height: delegateRoot.height
                    readonly property bool hiddenByOverflow: delegateRoot.isOverflowButton
                        || delegateRoot.hiddenInOverflow
                    property real overflowTransitionScale: hiddenByOverflow ? 0.8 : 1.0
                    scale: delegateRoot.scale * overflowTransitionScale
                    opacity: hiddenByOverflow ? 0 : 1
                    visible: opacity > 0.001
                    property bool positionAnimationEnabled: false

                    Behavior on opacity {
                        enabled: appItemRect.positionAnimationEnabled
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on overflowTransitionScale {
                        enabled: appItemRect.positionAnimationEnabled
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }

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
                        active: delegateRoot.active
                        attention: delegateRoot.attention
                        itemId: delegateRoot.itemId
                        name: delegateRoot.name
                        iconName: delegateRoot.iconName
                        menus: delegateRoot.menus
                        windows: delegateRoot.windows
                        visualIndex: delegateRoot.visualIndex
                        modelIndex: delegateRoot.modelIndex
                        blendOpacity: taskmanager.blendOpacity
                        title: delegateRoot.title
                        enableTitle: textCalculator.enabled
                        ListView.delayRemove: Drag.active
                        Component.onCompleted: {
                            dropFilesOnItem.connect(taskmanager.Applet.dropFilesOnItem)
                        }
                        onDragFinished: function() {
                            launcherDndDropArea.resetDndState()
                        }
                    }
                }
            }
        }

        TaskOverflowButton {
            id: overflowButton
            taskManagerItem: taskmanager
            overflow: taskOverflow
        }

        DropArea {
            id: launcherDndDropArea
            anchors.fill: parent
            z: 3
            keys: ["text/x-dde-dock-dnd-appid"]
            property string launcherDndDesktopId: ""
            property string launcherDndDragSource: ""
            property string launcherDndWinId: ""
            property bool launcherDndDocked: false

            function resetDndState() {
                launcherDndDesktopId = ""
                launcherDndDragSource = ""
                launcherDndWinId = ""
                launcherDndDocked = false
            }

            function isOverflowButtonIndex(targetIndex) {
                return taskOverflow.popupItems.length > 0
                    && targetIndex === taskOverflow.visibleItemCount
            }

            function targetIndexAt(x, y) {
                if (launcherDndDragSource !== "overflow-popup") {
                    return appContainer.indexAt(x, y)
                }

                const visibleCount = taskOverflow.visibleItemCount
                if (visibleCount <= 0) {
                    return -1
                }

                const itemExtent = taskOverflow.fallbackItemExtent
                const pitch = itemExtent + taskOverflow.spacing
                const position = taskmanager.useColumnLayout ? y : x
                const nearestSlot = Math.floor((Math.max(0, position) + taskOverflow.spacing / 2) / pitch)
                return Math.max(0, Math.min(visibleCount - 1, nearestSlot))
            }

            function isInternalDrag() {
                return launcherDndDragSource === "taskbar"
                    || launcherDndDragSource === "overflow-popup"
            }

            function requestDockIfNeeded() {
                if (isInternalDrag() || launcherDndDocked) {
                    return true
                }

                if (taskmanager.Applet.requestDockByDesktopId(launcherDndDesktopId) === false) {
                    resetDndState()
                    return false
                }

                launcherDndDocked = true
                return true
            }

            onEntered: function(drag) {
                let desktopId = drag.getDataAsString("text/x-dde-dock-dnd-appid")
                launcherDndDragSource = drag.getDataAsString("text/x-dde-dock-dnd-source")
                launcherDndWinId = drag.getDataAsString("text/x-dde-dock-dnd-winid")
                launcherDndDesktopId = desktopId
                if (launcherDndDragSource !== "taskbar" && taskmanager.Applet.requestDockByDesktopId(desktopId) === false) {
                    drag.accepted = false
                    resetDndState()
                let targetIndex = targetIndexAt(drag.x, drag.y)
                if (!isOverflowButtonIndex(targetIndex) && !requestDockIfNeeded()) {
                    return
                }
            }

            onPositionChanged: function(drag) {
                if (launcherDndDesktopId === "") return
                if (launcherDndDragSource === "overflow-popup") return
                let targetIndex = targetIndexAt(drag.x, drag.y)
                if (isOverflowButtonIndex(targetIndex) || !requestDockIfNeeded()) return
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.Applet.windowSplit ? taskmanager.findAppIndexByWindow(appId, launcherDndWinId) : taskmanager.findAppIndex(appId)
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    if (taskmanager.Applet.windowSplit) {
                        taskmanager.Applet.moveItem(currentIndex, targetIndex)
                    } else {
                        visualModel.items.move(currentIndex, targetIndex)
                        taskOverflow.scheduleSync()
                    }
                }
            }

            onDropped: function(drop) {
                Panel.contextDragging = false
                if (launcherDndDesktopId === "") return
                const overflowPopupDrag = launcherDndDragSource === "overflow-popup"
                let targetIndex = targetIndexAt(drop.x, drop.y)
                if (!requestDockIfNeeded()) return
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.Applet.windowSplit ? taskmanager.findAppIndexByWindow(appId, launcherDndWinId) : taskmanager.findAppIndex(appId)
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    if (taskmanager.Applet.windowSplit) {
                        taskmanager.Applet.moveItem(currentIndex, targetIndex)
                    } else {
                        visualModel.items.move(currentIndex, targetIndex)
                        taskOverflow.scheduleSync()
                    }
                }
                let appIds = []
                for (let i = 0; i < visualModel.items.count; i++) {
                    appIds.push(visualModel.items.get(i).model.itemId)
                }
                taskmanager.Applet.saveDockElementsOrder(appIds)
                if (overflowPopupDrag) {
                    drop.accept(Qt.MoveAction)
                }
                resetDndState()
            }

            onExited: function() {
                if (launcherDndDesktopId !== "" && !isInternalDrag() && launcherDndDocked) {
                    taskmanager.Applet.requestUndockByDesktopId(launcherDndDesktopId)
                }
                resetDndState()
            }
        }
    }


    // Solve for S (dockItemMaxSize). The forward layout equation is:
    //   totalSpace = appCount×S×itemRatio + gaps×max(10, S×spacingRatio)
    //              + max(0, spacing - S×k) + otherCount×S×multitaskViewIconRatio
    //
    // The max(10,…) makes this piecewise. Ignoring Math.round (continuous approx):
    //
    //   Regime 1 (S×spacingRatio ≥ 10): no clamp, every term ∝ S.
    //     Factor out S → S = totalSpace / sum_of_coefficients.
    //
    //   Regime 2 (S×spacingRatio < 10): spacing clamped to 10 (constant).
    //     startPad flips from S×(spacingRatio-k) to 10 - S×k.
    //     Separate constants from S-terms → S = (total - constants) / S_coefficient.
    function solveMaxSizeFull(totalSpace, appCount, otherCount) {
        if (appCount <= 0) {
            return totalSpace;
        }

        const spacingRatio = iconWidthToMaxSizeRatio / 3;                      // 9/14 / 3
        const k = (multitaskViewIconRatio - iconWidthToMaxSizeRatio) / 2;      // ≈ 0.0786
        const gaps = Math.max(0, appCount - 1);
        const otherRatio = otherCount > 0 ? otherCount * multitaskViewIconRatio : 0;

        // Regime 1: continuous spacing model
        const S = totalSpace / (appCount * iconWidthToMaxSizeRatio + gaps * spacingRatio + otherRatio + spacingRatio - k);

        // Regime 2: spacing clamped to 10
        if (S * spacingRatio < 10) {
            return Math.max(0, (totalSpace - gaps * 10 - 10) / (appCount * iconWidthToMaxSizeRatio - k + otherRatio));
        }
        return Math.max(0, S);
    }

    Component.onCompleted: {
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            const dockSize = Panel.rootObject.dockSize;
            const appCount = visualModel.count;

            if (appCount <= 0 || dockSize <= 0)
                return dockSize;

            if (textCalculator.enabled && textCalculator.totalWidth > 0)
                return dockSize;

            const otherItems = Math.max(0, Panel.rootObject.dockCenterPartCount - 1);
            const optimal = solveMaxSizeFull(Panel.rootObject.dockEffectiveCenterSpace, appCount, otherItems);
            const fitted = optimal >= dockSize ? dockSize : optimal;
            return Math.max(taskmanager.minimumDockItemSize, fitted);
        })
        taskOverflow.scheduleSync()
    }
}
