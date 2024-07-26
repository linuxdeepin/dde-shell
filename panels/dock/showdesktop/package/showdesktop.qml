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
    property bool needRecoveryWin: false
    property bool useColumnLayout: Panel.position % 2
    property int dockSize: Panel.rootObject.dockItemMaxSize
    property int dockOrder: 30
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : showDesktopWidth
    implicitHeight: useColumnLayout ? showDesktopWidth : Panel.rootObject.dockSize

    Rectangle {
        id: rectangle
        anchors.fill: parent
        color: "gray"
        opacity: 0.01
    }

    Control {
        anchors.left: parent.left
        anchors.top: parent.top
        implicitWidth: showdesktop.implicitWidth === showDesktopWidth ? 1 : showdesktop.implicitWidth
        implicitHeight: showdesktop.implicitWidth === showDesktopWidth ? showdesktop.implicitHeight : 1

        Rectangle {
            property D.Palette lineColor: DockPalette.showDesktopLineColor

            anchors.fill: parent
            color: D.ColorSelector.lineColor
        }
    }

    PanelToolTip {
        id: toolTip
        text: qsTr("show desktop")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onPressed: {
            toolTip.close()
            if (showdesktop.needRecoveryWin)
                showdesktop.needRecoveryWin = false
            else
                Applet.toggleShowDesktop()
            rectangle.opacity = 0.5
        }
        onReleased: (mouse)=> {
            if (0 <= mouse.x && mouse.x <= this.width && 0 <= mouse.y && mouse.y <= this.height)
                rectangle.opacity = 0.3
            else
                rectangle.opacity = 0.01
        }
        onEntered: {
            var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, Applet.rootObject.height / 2)
            toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
            toolTip.open()

            rectangle.opacity = 0.3
        }
        onExited: {
            if (showdesktop.needRecoveryWin)
                Applet.toggleShowDesktop()

            toolTip.close()
            rectangle.opacity = 0.01
        }
    }
}
