// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: launcher
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 12
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : Panel.rootObject.dockItemMaxSize
    implicitHeight: useColumnLayout ? Panel.rootObject.dockItemMaxSize : Panel.rootObject.dockSize

    Connections {
        target: Panel.rootObject
        function onDockCenterPartPosChanged()
        {
            updateLaunchpadPos()
        }
    }

    function updateLaunchpadPos()
    {
        var launchpad = DS.applet("org.deepin.ds.launchpad")
        if (!launchpad)
            return

        var lX = action.mapToItem(null, 0, 0).x
        var lY = Panel.rootObject.y
        launchpad.rootObject.windowedPos = Qt.point(lX, lY)
    }
    Component.onCompleted: {
        updateLaunchpadPos()
    }

    D.ActionButton {
        id: action
        anchors.centerIn: parent
        icon.name: Applet.iconName
        scale: Panel.rootObject.itemScale
        // 10 : 36 : 10
        icon.height: Panel.rootObject.itemIconSizeBase * 0.64
        icon.width: Panel.rootObject.itemIconSizeBase * 0.64
        onClicked: Applet.toggleLauncher()
        onXChanged: updateLaunchpadPos()
        onYChanged: updateLaunchpadPos()
    }
}
