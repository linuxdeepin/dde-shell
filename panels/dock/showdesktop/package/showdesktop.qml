// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0

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
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        implicitWidth: showdesktop.implicitWidth === showDesktopWidth ? 1 : showdesktop.implicitWidth
        implicitHeight: showdesktop.implicitWidth === showDesktopWidth ? showdesktop.implicitHeight : 1
        color: "gray"
        opacity: 0.5
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onPressed: {
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
            if (Applet.checkNeedShowDesktop()) {
                showdesktop.needRecoveryWin = true
                Applet.toggleShowDesktop()
            }
            rectangle.opacity = 0.3
        }
        onExited: {
            if (showdesktop.needRecoveryWin)
                Applet.toggleShowDesktop()
            rectangle.opacity = 0.01
        }
    }
}
