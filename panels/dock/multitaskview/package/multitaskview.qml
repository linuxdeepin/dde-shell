// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: toggleworkspace
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 13
    // 1:4 the distance between app : dock height; get width/height≈0.8
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : Panel.rootObject.dockItemMaxSize * 0.8
    implicitHeight: useColumnLayout ? Panel.rootObject.dockItemMaxSize * 0.8 : Panel.rootObject.dockSize

    PanelToolTip {
        id: toolTip
        text: qsTr("Open Workspace")
    }

    D.ActionButton {
        id: action
        anchors.centerIn: parent
        icon.name: Applet.iconName
        scale: Panel.rootObject.itemScale
        // 9:14 (iconSize/dockHeight)
        icon.height: Panel.rootObject.itemIconSizeBase * 0.643
        icon.width: Panel.rootObject.itemIconSizeBase * 0.643
        onClicked: {
            Applet.openWorkspace()
            toolTip.close()
        }
        onHoveredChanged: {
            if (hovered) {
                var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
                toolTip.toolTipX = point.x
                toolTip.toolTipY = point.y
                toolTip.open()
            } else {
                toolTip.close()
            }
        }
    }
}
