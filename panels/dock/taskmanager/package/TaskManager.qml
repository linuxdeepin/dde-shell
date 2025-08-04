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
    property int remainingSpacesForTaskManager: Panel.rootObject.dockLeftSpaceForCenter - Panel.rootObject.dockItemMaxSize * 1.2
    property int forceRelayoutWorkaround: 0

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

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : (Math.min(remainingSpacesForTaskManager, appContainer.implicitWidth) + forceRelayoutWorkaround)
    implicitHeight: useColumnLayout ? (Math.min(remainingSpacesForTaskManager, appContainer.implicitHeight) + forceRelayoutWorkaround) : Panel.rootObject.dockSize

    // Find target index by position using ListView's built-in indexAt method instead of relying on fixed width
    function findTargetIndexByPosition(dragPosition) {
        if (!appContainer || !appContainer.model || appContainer.model.count === 0) {
            return 0
        }
        
        // Use ListView's built-in indexAt method
        let targetIndex = appContainer.indexAt(dragPosition.x, dragPosition.y)
        
        // If no valid index found, return end position
        if (targetIndex === -1) {
            return appContainer.model.count
        }
        
        return targetIndex
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
            delegate: DropArea {
                id: delegateRoot
                required property bool active
                required property bool attention
                required property string itemId
                required property string name
                required property string iconName
                required property string menus
                required property list<string> windows
                keys: ["text/x-dde-dock-dnd-appid"]
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

                onEntered: function(drag) {
                    // TODO: this is actually unused, should change the delegateRoot type from DropArea to Item later.
                    visualModel.items.move(drag.source.DelegateModel.itemsIndex, delegateRoot.DelegateModel.itemsIndex)
                }

                property int visualIndex: DelegateModel.itemsIndex

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
                    ListView.delayRemove: Drag.active
                    Component.onCompleted: {
                        clickItem.connect(taskmanager.Applet.clickItem)
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
                let dragPosition = Qt.point(drag.x, drag.y)
                let targetIndex = taskmanager.findTargetIndexByPosition(dragPosition)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                taskmanager.Applet.dataModel.moveTo(appId, targetIndex)
            }

            onDropped: function(drop) {
                Panel.contextDragging = false
                if (launcherDndDesktopId === "") return
                let dropPosition = Qt.point(drop.x, drop.y)
                let targetIndex = taskmanager.findTargetIndexByPosition(dropPosition)
                let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
                taskmanager.Applet.dataModel.moveTo(appId, targetIndex)
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
