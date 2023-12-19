// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: launcher
    property int dockSize: Applet.parent.dockSize
    implicitWidth: dockSize
    implicitHeight: dockSize

    D.ActionButton {
        anchors.fill: parent
        icon.name: Applet.iconName
        onClicked: Applet.toggleLauncher()
    }
}
