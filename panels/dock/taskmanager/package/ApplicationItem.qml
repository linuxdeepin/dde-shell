// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: itemHolder
    required property bool useColumnLayout
    required property int cellWidth
    required property int dockHeight

    required property int displayMode
    required property int colorTheme
    required property bool active
    required property bool attention
    required property string itemId
    required property string name
    required property string iconName
    required property string menus
    required property list<string> windows
    required property int visualIndex
    required property var modelIndex

    AppItem {
        id: app
        displayMode: Panel.indicatorStyle
        colorTheme: Panel.colorTheme
        active: itemHolder.active
        attention: itemHolder.attention
        itemId: itemHolder.itemId
        name: itemHolder.name
        iconName: itemHolder.iconName
        menus: itemHolder.menus
        windows: itemHolder.windows
        visualIndex: itemHolder.visualIndex
        modelIndex: itemHolder.modelIndex
        ListView.delayRemove: Drag.active
        Component.onCompleted: {
            clickItem.connect(taskmanager.Applet.clickItem)
        }
        onDragFinished: function() {
            // launcherDndDropArea.resetDndState()
        }

        implicitWidth: useColumnLayout ? Panel.rootObject.dockItemMaxSize : itemHolder.cellWidth
        implicitHeight: useColumnLayout ? itemHolder.cellWidth : Panel.rootObject.dockItemMaxSize

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
