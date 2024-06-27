// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQml

import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray.quickpanel 1.0

Item {
    id: root
    implicitWidth: {
        return panelTrayItem.width
    }
    implicitHeight: {
        return panelTrayItem.height
    }
    property string trayItemPluginId: "sound"

    PanelTrayItem {
        id: panelTrayItem
        shellSurface: quickpanelModel.trayItemSurface
        trayQuickPanelItemSurface: quickpanelModel.trayQuickPanelItemSurface
        isOpened: popup.popupVisible
        onClicked: function () {
            console.log("show quickpanel")
            var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
            popup.popupX = point.x
            popup.popupY = Qt.binding(function () {
                return -popup.height - 10
            })
            popup.open()
        }
    }

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        QuickPanelPage {
            id: popupContent
            model: quickpanelModel
        }
    }

    QuickPanelModel {
        id: quickpanelModel
        sourceModel: DockCompositor.quickPluginSurfaces
        trayPluginModel: DockCompositor.trayPluginSurfaces
        trayItemPluginId: root.trayItemPluginId
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            console.log("popup created", popupSurface.popupType, popupSurface.pluginId)
            if (popupSurface.popupType === Dock.TrayPopupTypeEmbed) {
                quickpanelModel.requestShowSubPlugin(popupSurface.pluginId, popupSurface)
            }
        }
    }
}
