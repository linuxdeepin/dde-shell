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

    // 用于居中计算的实际应用区域尺寸
    property int appContainerWidth: useColumnLayout ? Panel.rootObject.dockSize : (appContainer.implicitWidth + forceRelayoutWorkaround)
    property int appContainerHeight: useColumnLayout ? (appContainer.implicitHeight + forceRelayoutWorkaround) : Panel.rootObject.dockSize

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
        }
        move: displaced
        model: DelegateModel {
            id: visualModel
            model: taskmanager.Applet.dataModel
            // 1:4 the distance between app : dock height; get width/height≈0.8
            property real cellWidth: Panel.rootObject.dockItemMaxSize * 0.8
            onCountChanged: function() {
                relayoutWorkaroundTimer.start()
            }
            delegate: Item {
                id: delegateRoot
                required property int index
                required property bool active
                required property bool attention
                required property string itemId
                required property string name
                required property string iconName
                required property string icon // winIconName
                required property string menus
                required property list<string> windows
                z: attention ? -1 : 0
                property bool visibility: itemId !== taskmanager.Applet.desktopIdToAppId(launcherDndDropArea.launcherDndDesktopId)

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

                // TODO: 临时溢出逻辑，待后面修改
                implicitWidth: useColumnLayout ? taskmanager.implicitWidth : visualModel.cellWidth
                implicitHeight: useColumnLayout ? visualModel.cellWidth : taskmanager.implicitHeight

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

                AppItem {
                    id: app
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
                    anchors.fill: parent // This is mandatory for draggable item center in drop area
                }
            }
        }

        DropArea {
            id: launcherDndDropArea
            anchors.fill: parent
            keys: ["text/x-dde-dock-dnd-appid"]
            property string launcherDndDesktopId: ""
            property string launcherDndDragSource: ""

            function resetDndState() {
                launcherDndDesktopId = ""
                launcherDndDragSource = ""
            }

            onEntered: function(drag) {
                let desktopId = drag.getDataAsString("text/x-dde-dock-dnd-appid")
                launcherDndDragSource = drag.getDataAsString("text/x-dde-dock-dnd-source")
                launcherDndDesktopId = desktopId
                if (launcherDndDragSource !== "taskbar" && taskmanager.Applet.requestDockByDesktopId(desktopId) === false) {
                    resetDndState()
                }
            }

            onPositionChanged: function(drag) {
                if (launcherDndDesktopId === "") return
                let targetIndex = appContainer.indexAt(drag.x, drag.y)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.findAppIndex(appId)
                if (currentIndex !== -1 && targetIndex !== -1 && currentIndex !== targetIndex) {
                    visualModel.items.move(currentIndex, targetIndex)
                }
            }

            onDropped: function(drop) {
                Panel.contextDragging = false
                if (launcherDndDesktopId === "") return
                let targetIndex = appContainer.indexAt(drop.x, drop.y)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                let currentIndex = taskmanager.findAppIndex(appId)
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
