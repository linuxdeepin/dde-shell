// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.quickpanel 1.0

QuickPanelProxyModel {
    id: root

    sourceModel: DockCompositor.quickPluginSurfaces
    // sourceModel: DockCompositor.trayPluginSurfaces

}
