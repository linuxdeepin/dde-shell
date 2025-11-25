// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

ContainmentItem {
    id: taskmanager
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 16
    property int remainingSpacesForTaskManager: Panel.itemAlignment === Dock.LeftAlignment ? Panel.rootObject.dockLeftSpaceForCenter : Panel.rootObject.dockRemainingSpaceForCenter
    property int forceRelayoutWorkaround: 0
    
    property int remainingSpacesForSplitWindow: Panel.rootObject.dockLeftSpaceForCenter - (Panel.rootObject.dockCenterPartCount - 1) * Panel.rootObject.dockItemMaxSize * 9 / 14
    // 用于居中计算的实际应用区域尺寸
    property int appContainerWidth: useColumnLayout ? Panel.rootObject.dockSize : (appContainer.implicitWidth + forceRelayoutWorkaround)
    property int appContainerHeight: useColumnLayout ? (appContainer.implicitHeight + forceRelayoutWorkaround) : Panel.rootObject.dockSize
    
    // 动态字符限制数组，存储每个应用的最大显示字符数
    property var dynamicCharLimits: []

    Timer {
        // FIXME: dockItemMaxSize(visualModel.cellWidth,actually its implicitWidth/Height) change will cause all delegate item's position change, but
        //        the newly added item will using the old cellWidth to calculate its position, thus it will be placed in the wrong position. Also it
        //        seems forceLayout() simply doesn't work, so we use a workaround here to force relayout the ListView inside the OverflowContainer.
        //        See: QTBUG-133953
        id: relayoutWorkaroundTimer
        interval: 250 // should longer than OverflowContainer.add duration
        repeat: false
        onTriggered: {
            taskmanager.forceRelayoutWorkaround = visualModel.count % 2 + 1
            console.log("force relayout", taskmanager.forceRelayoutWorkaround)
        }
    }

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : (Math.max(remainingSpacesForTaskManager, appContainer.implicitWidth) + forceRelayoutWorkaround)
    implicitHeight: useColumnLayout ? (Math.max(remainingSpacesForTaskManager, appContainer.implicitHeight) + forceRelayoutWorkaround) : Panel.rootObject.dockSize

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
    TextMetrics {
        id: textMetrics
        font.family: D.DTK.fontManager.t5.family
    }

    // 使用 TextMetrics 计算文本宽度
    function calculateTextWidth(text, textSize) {
        if (!text || text.length === 0) return 0
        textMetrics.font.pixelSize = textSize
        textMetrics.text = text
        //+4 for padding 保持跟appitemwithtitle显示的大小一致 否则UI上显示会溢出
        return textMetrics.advanceWidth + 4
    }

    // 计算文本在给定宽度下的最大字符数
    function calculateMaxCharsWithinWidth(title, maxWidth, textSize) {
        if (!title || title.length === 0) return 0
        let low = 1
        let high = title.length
        let result = 0
        while (low <= high) {
            let mid = Math.floor((low + high) / 2)
            let sub = title.substring(0, mid)
            let width = calculateTextWidth(sub, textSize)
            if (width <= maxWidth) {
                result = mid
                low = mid + 1
            } else {
                high = mid - 1
            }
        }
        return result
    }

    // 计算单个应用的显示宽度  iconsize + titlewidth
    function calculateItemWidth(title, maxChars, iconSize, textSize) {
        if (!title || title.length === 0) {
            return iconSize + 4
        }
        
        // maxCharLimit 为 0 时不显示文字
        if (maxChars <= 0) {
            return iconSize + 4
        }
        
        let titleLength = title.length
        let displayLen = 0
        
        if (titleLength <= maxChars) {
            displayLen = titleLength
        } else {
            displayLen = maxChars
        }
        
        if (displayLen <= 0) {
            return iconSize + 4
        }
        
        let text = ""
        if (titleLength > maxChars) {
            text = title.substring(0, displayLen) + "…"
        } else {
            text = title.substring(0, displayLen)
        }
        
        let textWidth = calculateTextWidth(text, textSize)
        return iconSize + textWidth + 8
    }

    // 计算所有应用的总宽度
    function calculateTotalWidth(charLimits, iconSize, textSize) {
        let count = visualModel.items.count
        if (count === 0) return 0
        
        let totalAppWidth = 0
        for (let i = 0; i < count; i++) {
            const item = visualModel.items.get(i)
            let maxChars = charLimits[i] !== undefined ? charLimits[i] : 7
            totalAppWidth += calculateItemWidth(item.model.title, maxChars, iconSize, textSize)
        }
        
        // 加上应用之间的间距
        let spacing = Panel.rootObject.itemSpacing + (count % 2)
        let totalSpacing = Math.max(0, count - 1) * spacing
        
        return totalAppWidth + totalSpacing
    }

    // 找出当前显示字符数最多的应用索引 
    function findLongestTitleIndex(charLimits) {
        let maxIdx = -1
        let maxChars = -1
        for (let i = 0; i < visualModel.items.count; i++) {
            let currentLimit = charLimits[i] !== undefined ? charLimits[i] : 7
            if (currentLimit > maxChars) {
                maxChars = currentLimit
                maxIdx = i
            }
        }
        return maxIdx
    }

    // 动态计算每个应用的字符限制数组
    function calculateDynamicCharLimits(remainingSpace, iconSize, textSize) {
        if (visualModel.items.count === 0) {
            return []
        }

        // 初始化：所有应用都按7个汉字宽度计算
        let charLimits = []
        let maxTitleWidth = calculateTextWidth("计算七个字长度", textSize)
        for (let i = 0; i < visualModel.items.count; i++) {
            const item = visualModel.items.get(i)
            let title = item.model.title || ""
            if (title.length === 0) {
                charLimits[i] = 0
            } else {
                charLimits[i] = calculateMaxCharsWithinWidth(title, maxTitleWidth, textSize)
            }
        }

        // 计算总宽度
        let totalWidth = calculateTotalWidth(charLimits, iconSize, textSize)

        // 如果总宽度超过剩余空间，逐步缩减最长标题
        while (totalWidth > remainingSpace) {
            let longestIdx = findLongestTitleIndex(charLimits)
            
            if (longestIdx === -1) {
                // 所有标题都已缩减到0，无法再缩减
                break
            }

            // 缩减该标题的字符数
            charLimits[longestIdx] = charLimits[longestIdx] - 1
            if (charLimits[longestIdx] < 0) {
                charLimits[longestIdx] = 0
            }

            // 重新计算总宽度
            totalWidth = calculateTotalWidth(charLimits, iconSize, textSize)
        }
        //过滤掉字符数为1的，因为一个字符+省略号，不美观 直接全部显示相应的图标
        for (let i = 0; i < visualModel.items.count; i++) {
            const item = visualModel.items.get(i)
            let title = item.model.title || ""
            let maxChars = charLimits[i] !== undefined ? charLimits[i] : 7
            if (maxChars === 1 && title.length > 1) {
                charLimits[i] = 0
            }
        }

        return charLimits
    }
    function updateDynamicCharLimits() {
        if (!taskmanager.Applet.windowSplit || taskmanager.useColumnLayout) {
            taskmanager.dynamicCharLimits = []
            return
        }

        if (!(Panel.position === Dock.Bottom || Panel.position === Dock.Top)) {
            taskmanager.dynamicCharLimits = []
            return
        }

        let iconSize = Panel.rootObject.dockItemMaxSize * 9 / 14
        let textSize = Math.max(10, Math.min(20, Math.round(iconSize * 0.35)))
        taskmanager.dynamicCharLimits = calculateDynamicCharLimits(taskmanager.remainingSpacesForSplitWindow, iconSize, textSize)
    }

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        useColumnLayout: taskmanager.useColumnLayout
        spacing: Panel.rootObject.itemSpacing + visualModel.count % 2
        add: Transition {
            NumberAnimation {
                properties: "scale,opacity"
                from: 0
                to: 1
                duration: 200
            }
        }
        remove: Transition {
            NumberAnimation {
                properties: "scale,opacity"
                from: 1
                to: 0
                duration: 200
            }
        }
        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                properties: "scale"
                to: 1
                duration: 200
            }
            NumberAnimation {
                properties: "opacity"
                to: 1
                duration: 200
            }
        }
        move: displaced
        model: DelegateModel {
            id: visualModel
            model: taskmanager.Applet.dataModel
            // 1:4 the distance between app : dock height; get width/height≈0.8
            property real cellWidth: Panel.rootObject.dockItemMaxSize * 0.8
            onCountChanged: function() {
                relayoutWorkaroundTimer.start()
                DS.singleShot(300, updateDynamicCharLimits)
            }
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
                    // 同一个应用，在 windowSplit 模式下需要检查窗口ID
                    if (taskmanager.Applet.windowSplit) {
                        if (launcherDndDropArea.launcherDndWinId) {
                            // 拖拽的是具体窗口：只隐藏该窗口，显示其他窗口和驻留图标
                            return windows.length === 0 || windows[0] !== launcherDndDropArea.launcherDndWinId
                        } else {
                            // 拖拽的是驻留图标（无窗口ID）：只隐藏驻留图标，显示运行中的窗口
                            return windows.length > 0
                        }
                    }
                    // 非 windowSplit 模式，隐藏整个应用
                    return false
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

                property int dynamicCharLimit: {
                    if (!taskmanager.Applet.windowSplit || useColumnLayout) {
                        return 7
                    }
                    if (taskmanager.dynamicCharLimits && DelegateModel.itemsIndex < taskmanager.dynamicCharLimits.length) {
                        return taskmanager.dynamicCharLimits[DelegateModel.itemsIndex]
                    }
                    return 7
                }

                implicitWidth: useColumnLayout ? taskmanager.implicitWidth :
                                (taskmanager.Applet.windowSplit && (Panel.position == Dock.Bottom || Panel.position == Dock.Top)
                                     ? (appLoader.item && appLoader.item.actualWidth ? appLoader.item.actualWidth : visualModel.cellWidth)
                                     : visualModel.cellWidth)
                implicitHeight: useColumnLayout ? visualModel.cellWidth : taskmanager.implicitHeight

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

                Loader {
                    id: appLoader
                    anchors.fill: parent
                    sourceComponent: (taskmanager.Applet.windowSplit && (Panel.position == Dock.Bottom || Panel.position == Dock.Top)) ? appItemWithTitleComponent : appItemComponent
                    
                    Component {
                        id: appItemComponent
                        AppItem {
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
                            ListView.delayRemove: Drag.active
                            Component.onCompleted: {
                                dropFilesOnItem.connect(taskmanager.Applet.dropFilesOnItem)
                            }
                            onDragFinished: function() {
                                launcherDndDropArea.resetDndState()
                            }
                        }
                    }
                    
                    Component {
                        id: appItemWithTitleComponent
                        AppItemWithTitle {
                            displayMode: Panel.indicatorStyle
                            colorTheme: Panel.colorTheme
                            active: delegateRoot.active
                            attention: delegateRoot.attention
                            itemId: delegateRoot.itemId
                            name: delegateRoot.name
                            windowTitle: delegateRoot.title
                            iconName: delegateRoot.iconName
                            menus: delegateRoot.menus
                            windows: delegateRoot.windows
                            visualIndex: delegateRoot.visualIndex
                            modelIndex: delegateRoot.modelIndex
                            maxCharLimit: delegateRoot.dynamicCharLimit
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
        }

        DropArea {
            id: launcherDndDropArea
            anchors.fill: parent
            keys: ["text/x-dde-dock-dnd-appid"]
            property string launcherDndDesktopId: ""
            property string launcherDndDragSource: ""
            property string launcherDndWinId: ""

            function resetDndState() {
                launcherDndDesktopId = ""
                launcherDndDragSource = ""
                launcherDndWinId = ""
            }

            onEntered: function(drag) {
                let desktopId = drag.getDataAsString("text/x-dde-dock-dnd-appid")
                launcherDndDragSource = drag.getDataAsString("text/x-dde-dock-dnd-source")
                launcherDndWinId = drag.getDataAsString("text/x-dde-dock-dnd-winid")
                launcherDndDesktopId = desktopId
                if (launcherDndDragSource !== "taskbar" && taskmanager.Applet.requestDockByDesktopId(desktopId) === false) {
                    resetDndState()
                }
            }

            onPositionChanged: function(drag) {
                if (launcherDndDesktopId === "") return
                let targetIndex = appContainer.indexAt(drag.x, drag.y)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.Applet.windowSplit ? taskmanager.findAppIndexByWindow(appId, launcherDndWinId) : taskmanager.findAppIndex(appId)
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    visualModel.items.move(currentIndex, targetIndex)
                }
            }

            onDropped: function(drop) {
                Panel.contextDragging = false
                if (launcherDndDesktopId === "") return
                let targetIndex = appContainer.indexAt(drop.x, drop.y)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.Applet.windowSplit ? taskmanager.findAppIndexByWindow(appId, launcherDndWinId) : taskmanager.findAppIndex(appId)
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    visualModel.items.move(currentIndex, targetIndex)
                }
                let appIds = []
                for (let i = 0; i < visualModel.items.count; i++) {
                    appIds.push(visualModel.items.get(i).model.itemId)
                }
                taskmanager.Applet.saveDockElementsOrder(appIds)
                resetDndState()
            }

            onExited: function() {
                if (launcherDndDesktopId !== "" && launcherDndDragSource !== "taskbar") {
                    taskmanager.Applet.requestUndockByDesktopId(launcherDndDesktopId)
                }
                resetDndState()
            }
        }
    }

    //windowSplit下：计算标签长度过程中会导致图标卡顿，挤在一起，计算完成后刷新布局
    Timer {
        id: windowSplitRelayoutTimer
        interval: 500
        repeat: false
        onTriggered: {
            updateDynamicCharLimits()
            taskmanager.forceRelayoutWorkaround = (visualModel.count + 1) % 2 + 1
        }
    }

    Connections {
        target: taskmanager.Applet
        function onWindowSplitChanged() {
            windowSplitRelayoutTimer.start()
        }
    }

    Connections {
        target: taskmanager.Applet.dataModel
        function onDataChanged(topLeft, bottomRight, roles) {
            if (!taskmanager.Applet.windowSplit || taskmanager.useColumnLayout)
                return
            if (!(Panel.position === Dock.Bottom || Panel.position === Dock.Top))
                return
            DS.singleShot(300, updateDynamicCharLimits)
        }
    }

    // 监听 remainingSpacesForSplitWindow 变化
    onRemainingSpacesForSplitWindowChanged: {
        DS.singleShot(300, updateDynamicCharLimits)
    }

    Component.onCompleted: {
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            return Math.min(Panel.rootObject.dockSize, Panel.rootObject.dockLeftSpaceForCenter * 1.2 / (Panel.rootObject.dockCenterPartCount - 1 + visualModel.count) - 2)
        })
        if(taskmanager.Applet.windowSplit) 
            windowSplitRelayoutTimer.start()
    }
}
