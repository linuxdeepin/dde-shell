// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: clipboardItem
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 1
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30 
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize
    property bool shouldVisible: Applet.visible

    PanelToolTip {
        id: toolTip
        text: qsTr("Clipboard")
    }
    D.ToolButton {
        anchors.centerIn: parent
        width: 30
        height: 30
        icon.name: "clipboard"
        icon.width: 16
        icon.height: 16
        onClicked: {
            Applet.toggleClipboard()
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
