// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: launcher
    property bool useColumnLayout: Panel.position % 2
    property int dockSize: Panel.rootObject.dockItemMaxSize
    property int dockOrder: 12
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : dockSize
    implicitHeight: useColumnLayout ? dockSize : Panel.rootObject.dockSize

    D.ActionButton {
        anchors.centerIn: parent
        icon.name: Applet.iconName
        // 10 : 36 : 10
        icon.height: dockSize * 9 / 14
        icon.width: dockSize * 9 / 14
        onClicked: Applet.toggleLauncher()
    }
}
