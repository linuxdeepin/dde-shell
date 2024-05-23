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

AppletItem {
    id: tray

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : trayContainter.suggestedImplicitWidth
    implicitHeight: useColumnLayout ? trayContainter.suggestedImplicitHeight : Panel.rootObject.dockSize

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height
        
        property int defaultX: popup.x
        property int defaultY: popup.y

        onHeightChanged: {
            popup.y = popup.defaultY - popup.height
        }

        onWidthChanged: {
            popup.x = popup.defaultX - popup.width / 2
        }

        Item {
            anchors.fill: parent
            ShellSurfaceItem {
                id: popupContent
                anchors.centerIn: parent
                onSurfaceDestroyed: {
                    popup.close()
                }
            }
        }
    }

    GridLayout {
        id: overflowId
        columns: 1
        rows: 1
        anchors.centerIn: parent
        flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
        property bool trayVisible: true
        columnSpacing: 10
        rowSpacing: 10
        OverflowContainer {
            id: trayContainter
            useColumnLayout: tray.useColumnLayout
            model: DockCompositor.trayPluginSurfaces
            spacing: 10
            delegate: Item {
                id: pluginItem
                property var plugin: modelData
                property var popupShow: false
                // TODO: should follow size size pilocy
                implicitHeight: 16
                implicitWidth: 16

                ShellSurfaceItem {
                    anchors.centerIn: parent
                    shellSurface: plugin
                    width: pluginItem.implicitWidth
                    height: pluginItem.implicitHeight
                }

                Component.onCompleted: {
                    var pos = pluginItem.mapToItem(null, 0, 0)
                    plugin.updatePluginGeometry(Qt.rect(pos.x, pos.y, 16, 16))
                }
            }
        }
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            // TODO: position need to recalucate
            popupContent.shellSurface = popupSurface
            popup.defaultX = popupSurface.x - (popup.width / 2)
            popup.defaultY = - 10
            popup.open()
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.rootObject
        sizeFollowsWindow: true
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.popupWindow
        sizeFollowsWindow: false
    }
}