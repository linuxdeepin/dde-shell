// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: showdesktop
    implicitWidth: 40
    implicitHeight: 40

    D.ActionButton {
        id: showdesktopButtom
        anchors.fill: parent
        icon.name: Applet.iconName
        onClicked: Applet.toggleShowDesktop()
    }
}
