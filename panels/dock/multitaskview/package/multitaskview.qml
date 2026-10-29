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

    Accessible.role: Accessible.Button
    Accessible.name: toolTip.text

    PanelToolTip {
        Accessible.role: Accessible.ToolTip
        Component.onCompleted: { Accessible.id = "MultitaskingView" }
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

    readonly property bool useCompositorTooltip: Qt.platform.pluginName === "wayland"
    property bool compositorTooltipActive: false
    function compositorTooltipApplet() {
        if (!useCompositorTooltip)
            return null

        const applets = DS.appletList("org.deepin.ds.dock.taskmanager")
        for (let i = 0; i < applets.length; ++i) {
            if (applets[i].parent === Applet.parent)
                return applets[i]
        }
        return null
    }

    function hideTooltip() {
        if (toolTipShowTimer.running)
            toolTipShowTimer.stop()

        if (compositorTooltipActive) {
            compositorTooltipActive = false
            var applet = compositorTooltipApplet()
            if (applet)
                applet.hideItemPreview()
        } else {
            toolTip.close()
        }
    }

    Timer {
        id: toolTipShowTimer
        interval: 50
        onTriggered: {
            var applet = compositorTooltipApplet()
            if (applet) {
                var itemPos = toggleworkspace.mapToItem(null, 0, 0)
                var xOffset, yOffset, interval = 10
                if (!useColumnLayout) {
                    xOffset = itemPos.x + toggleworkspace.width / 2
                    yOffset = (Panel.position == Dock.Bottom ? -interval : interval + Panel.dockSize)
                } else {
                    xOffset = (Panel.position == Dock.Right ? -interval : interval + Panel.dockSize)
                    yOffset = itemPos.y + toggleworkspace.height / 2
                }

                compositorTooltipActive = false
                if (applet.requestTooltip(toolTip.text, Panel.rootObject, xOffset, yOffset, Panel.position)) {
                    compositorTooltipActive = true
                    return
                }
                // 合成器协议不可用时回退到原生 tooltip，避免 tooltip 直接消失
            }

            var point = toggleworkspace.mapToItem(null, toggleworkspace.width / 2, toggleworkspace.height / 2)
            toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
            toolTip.open()
        }
    }

    TapHandler {
        id: tapHandler
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        gesturePolicy: TapHandler.WithinBounds
        onTapped: function (eventPoint, buttons) {
            if (buttons === Qt.LeftButton) {
                Applet.openWorkspace()
                hideTooltip()
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.NoButton
        acceptedDevices: PointerDevice.TouchScreen
        gesturePolicy: TapHandler.WithinBounds
        onTapped: function (eventPoint, buttons) {
            Applet.openWorkspace()
            hideTooltip()
        }
    }
    HoverHandler {
        onHoveredChanged: {
            if (hovered) {
                toolTipShowTimer.start()
            } else {
                hideTooltip()
            }
        }
    }
}
