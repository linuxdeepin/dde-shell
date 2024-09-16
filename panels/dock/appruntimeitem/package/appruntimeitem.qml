// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: appruntimeitem
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 1
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize
    property bool shouldVisible: Applet.visible
    property D.Palette toolButtonColor: DockPalette.toolButtonColor
    property D.Palette toolButtonBorderColor: DockPalette.toolButtonBorderColor

    PanelToolTip {
        id: toolTip
        text: qsTr("app_runtime")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }
    AppletItemButton {
        id: button
        anchors.centerIn: parent
        icon.name: "qrc:/ddeshell/package/icons/appruntime.svg"

        isActive: Applet.appruntimeVisible

        onClicked: {
            Applet.toggleruntimeitem()
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
    }
}
