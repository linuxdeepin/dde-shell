// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.private 1.0 as DP
import org.deepin.ds.dock 1.0

AppletItem {
    id: root
    property int dockOrder
    property bool shouldVisible
    property alias toolButtonColor: backgroundButton.color1
    property alias toolButtonBorderColor: backgroundButton.outsideBorderColor
    property alias toolTip: toolTip.text
    property alias icon: button.icon.name

    signal clicked

    PanelToolTip {
        id: toolTip
    }
    D.ToolButton {
        id: button
        anchors.centerIn: parent
        width: 30
        height: 30
        icon.width: 16
        icon.height: 16

        display: D.IconLabel.IconOnly
        onClicked: {
            toolTip.close()
            root.clicked()
        }

        onHoveredChanged: {
            if (toolTip.text === "")
                return

            if (hovered) {
                var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
                toolTip.toolTipX = point.x
                toolTip.toolTipY = point.y
                toolTip.open()
            } else {
                toolTip.close()
            }
        }

        background: DP.ButtonPanel {
            id: backgroundButton

            button: button
            color2: color1
            insideBorderColor: null
        }
    }
}
