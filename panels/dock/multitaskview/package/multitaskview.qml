// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletDockItem {
    id: toggleworkspace
    dockOrder: 15

    PanelToolTip {
        id: toolTip
        text: qsTr("Multitasking View")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    D.DciIcon {
        id: icon
        anchors.centerIn: parent
        name: Applet.iconName
        scale: Panel.rootObject.dockItemMaxSize * 9 / 14 / Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE
        // 9:14 (iconSize/dockHeight)
        sourceSize: Qt.size(Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE, Dock.MAX_DOCK_TASKMANAGER_ICON_SIZE)
    }

    Timer {
        id: toolTipShowTimer
        interval: 50
        onTriggered: {
            var point = toggleworkspace.mapToItem(null, toggleworkspace.width / 2, toggleworkspace.height / 2)
            toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
            toolTip.open()
        }
    }

    MouseArea {
        id: mouseHandler
        anchors.fill: parent
        onClicked: function (mouse) {
            if (mouse.button === Qt.LeftButton) {
                Applet.openWorkspace()
                toolTip.close()
            }
        }
    }
    HoverHandler {
        onHoveredChanged: {
            if (hovered) {
                toolTipShowTimer.start()
            } else {
                if (toolTipShowTimer.running) {
                    toolTipShowTimer.stop()
                }

                toolTip.close()
            }
        }
    }
}
