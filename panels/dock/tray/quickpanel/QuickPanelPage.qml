// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

import org.deepin.ds.dock 1.0
import org.deepin.ds 1.0
import org.deepin.dtk 1.0

Item {
    id: root
    width: panelView.width
    height: panelView.height

    required property var model

    Connections {
        target: model
        function onRequestShowSubPlugin(pluginId, surface) {
            console.log("show subplugin, plugin", pluginId)
            panelView.push(subPluginPageLoader,
                           {
                               pluginId: pluginId,
                               model: root.model,
                               shellSurface: surface,
                               subPluginMinHeight: Math.max(360, panelPage.height)
                           },
                           StackView.PushTransition)
        }
    }

    StackView {
        id: panelView
        width: currentItem.width
        height: currentItem.height
        initialItem: PanelPluginPage {
            id: panelPage
            model: root.model
        }
    }

    Component {
        id: subPluginPageLoader
        SubPluginPage {
            onRequestBack: function () {
                panelView.pop()
            }
        }
    }

    PanelPopupWindow {
        id: toolTipWindow
    }
    PanelToolTip {
        id: toolTip
        toolTipWindow: toolTipWindow

        property alias shellSurface: surfaceLayer.shellSurface
        ShellSurfaceItem {
            id: surfaceLayer
            anchors.centerIn: parent
            onSurfaceDestroyed: {
                toolTip.close()
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: toolTip.toolTipWindow
        sizeFollowsWindow: false
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            if (!model.isQuickPanelPopup(popupSurface.pluginId, popupSurface.itemKey))
                return

            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                console.log("quickpanel's tooltip created", popupSurface.popupType, popupSurface.pluginId)

                toolTip.shellSurface = popupSurface
                toolTip.toolTipX = popupSurface.x
                toolTip.open()
            }
        }
    }
}
