// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
// import org.deepin.ds.dockshell 1.0

DockItem {
    dockOrder: 1
    shouldVisible: Applet.visible
    toolButtonColor: DockPalette.toolButtonColor
    toolButtonBorderColor: DockPalette.toolButtonBorderColor
    toolTip: "I am tips"
    icon: Applet.icon === "" ? "x-application-desktop" : Applet.icon

    property int dockSize: Panel.rootObject.dockSize
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize

    onClicked: {
        DDockApplet.activate()
    }
}
