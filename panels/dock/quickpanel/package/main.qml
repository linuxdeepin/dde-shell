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

AppletItem {
    id: root
    implicitWidth: {
        return panelTrayItem.width
    }
    implicitHeight: {
        return panelTrayItem.height
    }

    PanelTrayItem {
        id: panelTrayItem
        shellSurface: quickpanelModel.traySurfaceItem
        onClicked: {
            console.log("show quickpanel")
            var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
            popup.x = point.x
            popup.y = point.y
            popup.open()
        }
    }

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

        QuickPanel {
            id: popupContent
            model: quickpanelModel
        }
    }

    QuickPanelModel {
        id: quickpanelModel
        trayItemPluginName: "sound-item-key"
    }

}
