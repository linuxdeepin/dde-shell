// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: taskmanager
    property bool useColumnLayout: Applet.parent.position % 2
    property int dockSize: Applet.parent.dockSize
    property int dockOrder: 15

    implicitWidth: useColumnLayout ? dockSize : appContainer.implicitWidth
    implicitHeight: useColumnLayout ? appContainer.implicitHeight : dockSize

    OverflowContainer {
        id: appContainer
        anchors.fill: parent
        useColumnLayout: taskmanager.useColumnLayout
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
                required property string itemId
                required property string name
                required property string iconName
                required property string menus
                required property list<string> windows

                implicitWidth: dockSize
                implicitHeight: dockSize

                onEntered: function(drag) {
                    visualModel.items.move((drag.source as AppItem).visualIndex, app.visualIndex)
                }

                onDropped: function(drop) {
                    // FIXME: Dragging doesn't end on dropped even if Drag.onDragFinished is executed
                    drop.accept()
                }

                property int visualIndex: DelegateModel.itemsIndex

                AppItem {
                    id: app
                    displayMode: taskmanager.Applet.parent.displayMode
                    colorTheme: taskmanager.Applet.parent.colorTheme
                    active: delegateRoot.active
                    itemId: delegateRoot.itemId
                    name: delegateRoot.name
                    iconName: delegateRoot.iconName
                    menus: delegateRoot.menus
                    windows: delegateRoot.windows
                    visualIndex: delegateRoot.visualIndex
                    Component.onCompleted: {
                        clickItem.connect(taskmanager.Applet.clickItem)
                        clickItemMenu.connect(taskmanager.Applet.clickItemMenu)
                    }
                    anchors.fill: parent // This is mandatory for draggable item center in drop area
                }
            }
        }
    }
}
