// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.private 1.0 as DP
import org.deepin.ds.dock 1.0
import org.deepin.dtk.style 1.0 as DStyle

AppletItem {
    id: clipboardItem
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 1
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize
    property bool shouldVisible: Applet.visible
    property D.Palette toolButtonColor: DockPalette.toolButtonColor
    property D.Palette toolButtonBorderColor: DockPalette.toolButtonBorderColor

    PanelToolTip {
        id: toolTip
        text: qsTr("Clipboard")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }
    D.ToolButton {
        id: button
        anchors.centerIn: parent
        width: 30
        height: 30
        icon.name: "clipboard"
        icon.width: 16
        icon.height: 16

        D.ColorSelector.hovered: Applet.clipboardVisible || button.hovered

        display: D.IconLabel.IconOnly
        onClicked: {
            Applet.toggleClipboard()
            toolTip.close()
        }

        onHoveredChanged: {
            if (hovered) {
                var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, Applet.rootObject.height / 2)
                toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                toolTip.open()
            } else {
                toolTip.close()
            }
        }

        background: D.BoxPanel {
            property D.Palette backgroundPalette: DockPalette.backgroundPalette

            color2: color1
            color1: backgroundPalette

            outsideBorderColor: null
            insideBorderColor: null
        }
    }
}
