// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: showdesktop
    readonly property int showDesktopWidth: 10
    property bool useColumnLayout: Panel.position % 2
    readonly property bool adaptiveFashionMode: Panel.position === Dock.Bottom && Panel.viewMode === Dock.FashionMode
    property int dockSize: Panel.rootObject.dockItemMaxSize
    property int dockOrder: 30
    property bool shouldVisible: Applet.visible && !adaptiveFashionMode
    visible: shouldVisible
    implicitWidth: shouldVisible ? (useColumnLayout ? Panel.rootObject.dockSize : showDesktopWidth) : 0
    implicitHeight: shouldVisible ? (useColumnLayout ? showDesktopWidth : Panel.rootObject.dockSize) : 0

    PanelToolTip {
        id: toolTip
        text: qsTr("show desktop")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    AppletItemButton {
        anchors.fill: parent
        radius: 0

        Rectangle {
            property D.Palette lineColor: DockPalette.showDesktopLineColor
            // Use device pixel ratio to ensure the line is always 1 physical pixel regardless of system scaling
            property real devicePixelRatio: Screen.devicePixelRatio
            anchors.centerIn: parent
            implicitWidth: useColumnLayout ? showdesktop.implicitWidth : (1 / devicePixelRatio)
            implicitHeight: useColumnLayout ? (1 / devicePixelRatio) : showdesktop.implicitHeight

            color: D.ColorSelector.lineColor
        }

        onClicked: {
            toolTip.close()
            Applet.toggleShowDesktop()
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
