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

    readonly property int appTitleSpacing: Math.max(10, Math.round(Panel.rootObject.dockItemMaxSize * 9 / 14) / 3)
    property real remainingSpacesForSplitWindow: Panel.rootObject.adaptiveFashionMode ? 0 : Math.max(0, Panel.rootObject.dockLeftSpaceForCenter - (
        (Panel.rootObject.dockCenterPartCount - 1) * (visualModel.cellWidth + appTitleSpacing) + (Panel.rootObject.dockCenterPartCount) * Panel.rootObject.dockPartSpacing))
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
                        active: delegateRoot.active
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
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            if (Panel.rootObject.adaptiveFashionMode) {
                return Panel.rootObject.dockSize
            }

            return Math.min(Panel.rootObject.dockSize, Panel.rootObject.dockLeftSpaceForCenter * 1.2 / (Panel.rootObject.dockCenterPartCount - 1 + visualModel.count) - 2)
        })
    }
}
