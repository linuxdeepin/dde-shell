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
    property int dockOrder: 15

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : appContainer.implicitWidth
    implicitHeight: useColumnLayout ? appContainer.implicitHeight : Panel.rootObject.dockSize

    OverflowContainer {
        id: appContainer
        anchors.centerIn: parent
        useColumnLayout: taskmanager.useColumnLayout
        spacing: Panel.rootObject.itemSpacing
        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutQuad
            }
        }
        model: DelegateModel {
            id: visualModel
            model: taskmanager.Applet.dataModel
            delegate: DropArea {
                id: delegateRoot
                required property bool active
                required property bool attention
                required property string itemId
                required property string name
                required property string iconName
                required property string menus
                required property list<string> windows
                keys: ["text/x-dde-dock-dnd-appid", "text/x-dde-launcher-dnd-desktopId"]
                z: attention ? -1 : 0

                // TODO: 临时溢出逻辑，待后面修改
                // 1:4 the distance between app : dock height; get width/height≈0.8
                implicitWidth: useColumnLayout ? Panel.rootObject.dockItemMaxSize : Panel.rootObject.dockItemMaxSize * 0.8
                implicitHeight: useColumnLayout ? Panel.rootObject.dockItemMaxSize * 0.8 : Panel.rootObject.dockItemMaxSize

                onEntered: function(drag) {
                    if (drag.keys.includes("text/x-dde-launcher-dnd-desktopId")) {
                        // accepted but don't need to do anything here.
                        return;
                    }
                    visualModel.items.move((drag.source as AppItem).visualIndex, app.visualIndex)
                }

                onDropped: function(drop) {
                    if (drop.keys.includes("text/x-dde-launcher-dnd-desktopId")) {
                        let desktopId = drop.getDataAsString("text/x-dde-launcher-dnd-desktopId")
                        taskmanager.Applet.requestDockByDesktopId(desktopId)
                        drop.accepted = false
                        return;
                    }
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
                    Component.onCompleted: {
                        clickItem.connect(taskmanager.Applet.clickItem)
                    }

                    onDragFinished: function() {
                        // 就算在非法区域松开也更新 Model
                        taskmanager.Applet.dataModel.moveTo(itemId, visualIndex)

                        // 更新 visualModel 的 model 数据
                        visualModel.model = taskmanager.Applet.dataModel
                    }
                    anchors.fill: parent // This is mandatory for draggable item center in drop area
                }
            }
        }
    }

    Component.onCompleted: {
        Panel.rootObject.dockItemMaxSize = Qt.binding(function(){
            return Math.min(Panel.rootObject.dockSize, Panel.rootObject.dockLeftSpaceForCenter * 1.2 / (Panel.rootObject.dockCenterPartCount - 1 + taskmanager.Applet.dataModel.rowCount()) - 2)
        })
    }
}
