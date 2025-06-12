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
    clip: true
    Rectangle {
        id: contentItem
        anchors.fill: parent
        border.color: "green"
        color: "transparent"
        z: 999
    }

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        useColumnLayout: taskmanager.useColumnLayout
        interactive: true
        spacing: Panel.rootObject.itemSpacing
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
                required property int index
                required property bool active
                required property bool attention
                required property string itemId
                required property string name
                required property string icon
                required property string iconName
                required property string title
                required property string menus
                required property list<string> windows
                keys: ["text/x-dde-dock-dnd-appid"]
                z: attention ? -1 : 0
                property bool visibility: true//itemId !== taskmanager.Applet.desktopIdToAppId(launcherDndDropArea.launcherDndDesktopId)

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

                anchors.horizontalCenter: useColumnLayout ? parent.horizontalCenter : undefined

                // TODO: 临时溢出逻辑，待后面修改
                implicitWidth: itemHolder.width
                implicitHeight: itemHolder.height

                onEntered: function(drag) {
                    visualModel.items.move(drag.source.DelegateModel.itemsIndex, delegateRoot.DelegateModel.itemsIndex)
                }

                property int visualIndex: DelegateModel.itemsIndex
                property var modelIndex: visualModel.modelIndex(index)

                Row {
                    id: itemHolder
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
                            clickItem.connect(taskmanager.Applet.clickItem)
                            dropFilesOnItem.connect(taskmanager.Applet.dropFilesOnItem)
                        }
                        onDragFinished: function() {
                            // launcherDndDropArea.resetDndState()
                        }

                        implicitWidth: useColumnLayout ? Panel.rootObject.dockItemMaxSize : visualModel.cellWidth
                        implicitHeight: useColumnLayout ? visualModel.cellWidth : Panel.rootObject.dockItemMaxSize

                        Drag.source: delegateRoot
                    }
                    Label {
                        visible: taskmanager.Applet.windowSplit && !taskmanager.useColumnLayout && (delegateRoot.windows.length > 0)
                        anchors.verticalCenter: itemHolder.verticalCenter
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        width: Math.min(100, implicitWidth)
                        text: delegateRoot.title !== "" ? `${delegateRoot.title}(${delegateRoot.index})` : delegateRoot.name
                    }
                }
            }
        }

        Button {
            anchors.top: parent.top
            anchors.left: parent.left
            visible: !appContainer.atViewBeginning
            width: Panel.rootObject.dockSize
            height: Panel.rootObject.dockSize
            icon.name: useColumnLayout ? "arrow-up" : "arrow-left"
            onClicked: {
                appContainer.scrollDecrease()
            }
        }

        Button {
            anchors.top: useColumnLayout ? undefined : parent.top
            anchors.right: parent.right
            anchors.bottom: useColumnLayout ? parent.bottom : undefined
            visible: !appContainer.atViewEnd
            width: Panel.rootObject.dockSize
            height: Panel.rootObject.dockSize
            icon.name: useColumnLayout ? "arrow-down" : "arrow-right"
            onClicked: {
                appContainer.scrollIncrease()
            }
        }

        Component.onCompleted: {
            appContainer.forceLayout()
        }

        Timer {
            id: relayoutTimer
            interval: 2000
            running: false
            repeat: false
            onTriggered: {
                console.log("hit")
                appContainer.forceLayout()
            }
        }

        Connections {
            target: taskmanager.Applet
            function onWindowSplitChanged() {
                console.log("windowSplitChanged")
                relayoutTimer.restart()
            }
        }

        // DropArea {
        //     id: launcherDndDropArea
        //     anchors.fill: parent
        //     keys: ["text/x-dde-dock-dnd-appid"]
        //     property string launcherDndDesktopId: ""
        //     property string launcherDndDragSource: ""

        //     function resetDndState() {
        //         launcherDndDesktopId = ""
        //         launcherDndDragSource = ""
        //     }

        //     onEntered: function(drag) {
        //         let desktopId = drag.getDataAsString("text/x-dde-dock-dnd-appid")
        //         launcherDndDragSource = drag.getDataAsString("text/x-dde-dock-dnd-source")
        //         launcherDndDesktopId = desktopId
        //         if (taskmanager.Applet.requestDockByDesktopId(desktopId) === false) {
        //             resetDndState()
        //         }
        //     }

        //     onPositionChanged: function(drag) {
        //         if (launcherDndDesktopId === "") return
        //         let curX = taskmanager.useColumnLayout ? drag.y : drag.x
        //         let cellWidth = visualModel.cellWidth
        //         let curCell = curX / cellWidth
        //         let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
        //         taskmanager.Applet.dataModel.moveTo(appId, curCell)
        //     }

        //     onDropped: function(drop) {
        //         if (launcherDndDesktopId === "") return
        //         let curX = taskmanager.useColumnLayout ? drop.y : drop.x
        //         let cellWidth = visualModel.cellWidth
        //         let curCell = curX / cellWidth
        //         let appId = taskmanager.Applet.desktopIdToAppId(launcherDndDesktopId)
        //         taskmanager.Applet.dataModel.moveTo(appId, curCell)
        //         resetDndState()
        //     }

        //     onExited: function() {
        //         if (launcherDndDesktopId !== "" && launcherDndDragSource !== "taskbar") {
        //             taskmanager.Applet.requestUndockByDesktopId(launcherDndDesktopId)
        //         }
        //         resetDndState()
        //     }
        // }
    }

    Component.onCompleted: {
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            return Math.min(Panel.rootObject.dockSize, Panel.rootObject.dockLeftSpaceForCenter * 1.2 / (Panel.rootObject.dockCenterPartCount - 1 + visualModel.count) - 2)
        })
    }
}
