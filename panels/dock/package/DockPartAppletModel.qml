// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

D.SortFilterModel {
    id: model

    property int leftDockOrder: 0
    property int rightDockOrder: 0
    property var acceptItem: null
    property var sortOrderProvider: null

    model: Applet.appletItems

    filterAcceptsItem: function(item) {
        if (acceptItem) {
            return acceptItem(item)
        }
        return item.data.dockOrder > leftDockOrder && item.data.dockOrder <= rightDockOrder && (item.data.shouldVisible === undefined || item.data.shouldVisible)
    }
    lessThan: function(leftItem, rightItem) {
        const leftOrder = sortOrderProvider ? sortOrderProvider(leftItem) : parseInt(leftItem.data.dockOrder)
        const rightOrder = sortOrderProvider ? sortOrderProvider(rightItem) : parseInt(rightItem.data.dockOrder)
        if (leftOrder !== rightOrder) {
            return leftOrder < rightOrder
        }

        const leftDockOrder = parseInt(leftItem.data.dockOrder)
        const rightDockOrder = parseInt(rightItem.data.dockOrder)
        if (leftDockOrder !== rightDockOrder) {
            return leftDockOrder < rightDockOrder
        }

        const leftPluginId = leftItem.data && leftItem.data.applet ? leftItem.data.applet.pluginId : ""
        const rightPluginId = rightItem.data && rightItem.data.applet ? rightItem.data.applet.pluginId : ""
        return leftPluginId < rightPluginId
    }
    delegate: Control {
        id: delegateRoot

        property var appletItem: model.data
        property var attachedAppletItem: null

        contentItem: appletItem

        function syncAppletItem() {
            if (attachedAppletItem && attachedAppletItem !== appletItem && attachedAppletItem.parent === delegateRoot) {
                attachedAppletItem.parent = null
            }

            if (appletItem) {
                appletItem.parent = delegateRoot
                attachedAppletItem = appletItem
            } else {
                attachedAppletItem = null
            }
        }

        onAppletItemChanged: syncAppletItem()

        Component.onCompleted: syncAppletItem()

        Component.onDestruction: {
            if (attachedAppletItem && attachedAppletItem.parent === delegateRoot) {
                attachedAppletItem.parent = null
            }
            attachedAppletItem = null
        }
    }
}
