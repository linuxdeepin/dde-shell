// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

import WidgetProxy 1.0

AppletItem {
    id: tray
    visible: true
    readonly property int showDesktopWidth: 10
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    property var position: Panel.position
    property var indicatorStyle: Panel.indicatorStyle
    property var dockWidth: Applet.dockWidth
    property var dockHeight: Applet.dockHeight

    WidgetProxy {
        anchors.centerIn: parent
        Component.onCompleted: Applet.initDock()
    }

    function updatePanelGeometry() {
        var topLeft = this.mapToGlobal(Qt.point(0, 0))
        if (useColumnLayout) {
            Applet.setPanelSize(Window.width)
            Applet.setPanelPosition(topLeft.x, Window.height - dockHeight - showDesktopWidth)
        } else {
            Applet.setPanelSize(Window.height)
            Applet.setPanelPosition(Window.width - dockWidth - showDesktopWidth, topLeft.y)
        }
    }

    Window.onWindowChanged: {
        Applet.setDockPosition(Panel.position)
        Applet.setDisplayMode(Panel.indicatorStyle)
        updatePanelGeometry()
    }
    Window.onWidthChanged: updatePanelGeometry()
    Window.onHeightChanged: updatePanelGeometry()
    onPositionChanged: Applet.setDockPosition(Panel.position)
    onIndicatorStyleChanged: Applet.setDisplayMode(Panel.indicatorStyle)
    onDockWidthChanged: updatePanelGeometry()
    onDockHeightChanged: updatePanelGeometry()

    implicitWidth: dockWidth /*useColumnLayout ? Panel.dockSize : trayContainter.suggestedImplicitWidth + overflowBtn.implicitWidth * 2*/
    implicitHeight: dockHeight /*useColumnLayout ? trayContainter.suggestedImplicitHeight + overflowBtn.implicitHeight * 2 : Panel.dockSize*/
    Behavior on implicitWidth {
        SmoothedAnimation {
            velocity: useColumnLayout ? 100000:  600
        }
    }
    Behavior on implicitHeight {
        SmoothedAnimation {
            velocity: !useColumnLayout ? 100000 : 600
        }
    }
    PanelToolTip {
        id: tooltip
        width: tooltipContent.width
        height: tooltipContent.height

        ShellSurfaceItem {
            id: tooltipContent
        }
    }

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        ShellSurfaceItem {
            id: popupContent
        }
    }

    Timer {
        id: tooltipTimer
        interval: 300
        running: false
        repeat: false
        onTriggered: tooltip.open()
    }

    GridLayout {
        id: overflowId
        columns: 1
        rows: 1
        anchors.centerIn: parent
        flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
        property bool trayVisible: true
        OverflowContainer {
            id: trayContainter
            assignedWidth: -1
            assignedHeight: -1
            useColumnLayout: tray.useColumnLayout
            model: DockCompositor.trayPluginSurfaces
            spacing: 10
            Behavior on implicitWidth {
                SmoothedAnimation {
                    velocity: {
                        if (useColumnLayout) {
                            return 1000000
                        }
                        if (overflowId.trayVisible) {
                            return 300
                        }
                        return 1200
                    }
                }
            }
            Behavior on implicitHeight {
                SmoothedAnimation {
                    velocity: {
                        if (!useColumnLayout) {
                            return 1000000
                        }
                        if (overflowId.trayVisible) {
                            return 300
                        }
                        return 1200
                    }
                }
            }
            delegate: Item {
                id: pluginItem
                property var plugin: modelData
                property var popupShow: false
                implicitHeight: 16
                implicitWidth: 16

                ShellSurfaceItem {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    shellSurface: plugin

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onEntered: {
                            var toolTipsurface = DockCompositor.tooltipMap[plugin.pluginId]
                            if (toolTipsurface !== undefined) {
                                tooltipContent.shellSurface = toolTipsurface
                                var itemPos = tray.getItemPopupPosition(pluginItem, tooltip)
                                tooltip.x = itemPos.x
                                tooltip.y = itemPos.y
                                tooltipTimer.restart()
                            }
                        }

                        onClicked: mouse => {
                                       var pluginPopup = DockCompositor.popupMap[plugin.pluginId]
                                       if (pluginPopup === undefined) {
                                           plugin.click("", 0)
                                       } else {
                                           tooltip.close()
                                           popupContent.shellSurface = pluginPopup
                                           popupShow = true
                                           var itemPos = tray.getItemPopupPosition(pluginItem, popup)
                                           popup.x = itemPos.x
                                           popup.y = itemPos.y
                                           popup.open()
                                       }
                                   }

                        onExited: {
                            if (tooltipTimer.running) {
                                tooltipTimer.stop()
                            } else {
                                tooltip.close()
                            }
                        }
                    }
                }
            }
        }
        D.ActionButton {
            implicitWidth: 16
            implicitHeight: 16
            visible: false
            icon.name : {
                if (useColumnLayout) {
                    return overflowId.trayVisible ? "go-up" : "go-down"
                } else {
                    return overflowId.trayVisible ? "go-right" : "go-left"
                }
            }
            id: overflowBtn
            onClicked: {
                overflowId.trayVisible = !overflowId.trayVisible;
                if (overflowId.trayVisible) {
                    if (useColumnLayout) {
                        trayContainter.assignedHeight = -1
                        trayContainter.assignedWidth = -1
                    } else {
                        trayContainter.assignedWidth = -1
                        trayContainter.assignedHeight = -1
                    }
                } else {
                    if (useColumnLayout) {
                        trayContainter.assignedWidth = -1
                        trayContainter.assignedHeight = 0
                    } else {
                        trayContainter.assignedHeight = -1
                        trayContainter.assignedWidth = 0
                    }
                }
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.rootObject
        sizeFollowsWindow: true
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.toolTipWindow
        sizeFollowsWindow: true
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.popupWindow
        sizeFollowsWindow: true
    }

    function getItemPopupPosition(item, popupItem) {
        var itemPos = item.mapToItem(null,0,0)

        switch (Panel.position) {
        case Dock.Top:
            itemPos.x -= popupItem.width / 2
            itemPos.y += Panel.dockSize
            break
        case Dock.Right:
            itemPos.x -= (popupItem.width + 10)
            itemPos.y -= popupItem.height / 2
            break
        case Dock.Bottom:
            itemPos.x -= popupItem.width / 2
            itemPos.y -= (popupItem.height + 10)
            break
        case Dock.Left:
            itemPos.x += Panel.dockSize
            itemPos.y -= popupItem.height / 2
            break
        }

        return itemPos
    }
}
