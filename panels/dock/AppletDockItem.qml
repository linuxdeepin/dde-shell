// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.ds 1.0

AppletItem {
    id: appletDockItem

    property int dockOrder: 0
    property bool shouldVisible: Applet.visible
    property bool useColumnLayout: Panel.position % 2
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : Panel.rootObject.dockItemMaxSize * 0.8
    implicitHeight: useColumnLayout ? Panel.rootObject.dockItemMaxSize * 0.8 : Panel.rootObject.dockSize

}
