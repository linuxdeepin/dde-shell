// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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
    property real remainingSpacesForTaskManager: Panel.itemAlignment === Dock.LeftAlignment ? Panel.rootObject.dockLeftSpaceForCenter : Panel.rootObject.dockRemainingSpaceForCenter

    property real remainingSpacesForSplitWindow: Panel.rootObject.dockLeftSpaceForCenter - (
        (Panel.rootObject.dockCenterPartCount - 1) * (visualModel.cellWidth + appContainer.spacing) + (Panel.rootObject.dockCenterPartCount) * Panel.rootObject.dockPartSpacing)
    // 用于居中计算的实际应用区域尺寸
    property int appContainerWidth: useColumnLayout ? Panel.rootObject.dockSize : appContainer.implicitWidth
    property int appContainerHeight: useColumnLayout ? appContainer.implicitHeight : Panel.rootObject.dockSize
    
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : Math.max(remainingSpacesForTaskManager, appContainer.implicitWidth)
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

    function blendColorAlpha(fallback) {
        var appearance = DS.applet("org.deepin.ds.dde-appearance")
        if (!appearance || appearance.opacity < 0)
            return fallback
        return appearance.opacity
    }
    property real blendOpacity: blendColorAlpha(D.DTK.themeType === D.ApplicationHelper.DarkType ? 0.25 : 1.0)

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

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        useColumnLayout: taskmanager.useColumnLayout
        spacing: Panel.rootObject.itemSpacing + visualModel.count % 2
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

                implicitWidth: useColumnLayout ? taskmanager.implicitWidth : appItem.implicitWidth
                implicitHeight: useColumnLayout ? visualModel.cellWidth : taskmanager.implicitHeight

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

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

    Component.onCompleted: {
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            return Math.min(Panel.rootObject.dockSize, Panel.rootObject.dockLeftSpaceForCenter * 1.2 / (Panel.rootObject.dockCenterPartCount - 1 + visualModel.count) - 2)
        })
    }
}
