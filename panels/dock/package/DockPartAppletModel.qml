// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
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
        readonly property string pluginId: appletItem && appletItem.applet ? appletItem.applet.pluginId : ""
        readonly property bool isAiBarApplet: pluginId === "org.deepin.ds.dock.aibar"
        readonly property bool horizontalAiBarApplet: isAiBarApplet
            && Panel.viewMode === Dock.FashionMode
            && !(Panel.rootObject && Panel.rootObject.useColumnLayout)
        readonly property int hoverInset: 4
        readonly property bool useUnifiedDockHoverBackground: [
            "org.deepin.ds.dock.launcherapplet",
            "org.deepin.ds.dock.aibar",
            "org.deepin.ds.dock.searchitem",
            "org.deepin.ds.dock.multitaskview"
        ].indexOf(pluginId) >= 0
        readonly property real aiBarIconSize: appletItem && appletItem.iconWidth !== undefined ? appletItem.iconWidth : 0
        readonly property real hoverTargetWidth: isAiBarApplet && aiBarIconSize > 0
            ? aiBarIconSize
            : (Panel.rootObject ? Panel.rootObject.dockItemMaxSize * 9 / 14 : 0)
        readonly property real hoverTargetHeight: hoverTargetWidth
        readonly property real unifiedHoverBackgroundWidth: Math.round(hoverTargetWidth + hoverInset * 2)
        readonly property real unifiedHoverBackgroundHeight: Math.round(hoverTargetHeight + hoverInset * 2)
        readonly property real aiBarEdgeMargin: Panel.rootObject
            ? Math.max(0, (Panel.rootObject.dockSize - hoverTargetHeight) / 2)
            : 0
        readonly property real aiBarTrailingInset: Panel.rootObject ? Panel.rootObject.fashionVerticalPadding + 1 : 0
        readonly property real aiBarContentWidth: appletItem ? appletItem.implicitWidth : 0
        readonly property real aiBarLayoutWidth: horizontalAiBarApplet
            ? Math.max(aiBarContentWidth, hoverTargetWidth + aiBarEdgeMargin * 2 - aiBarTrailingInset)
            : aiBarContentWidth
        readonly property int aiBarHorizontalOffset: horizontalAiBarApplet
            ? Math.round(aiBarEdgeMargin)
            : 0
        readonly property real aiBarRightPadding: horizontalAiBarApplet
            ? Math.max(0, aiBarLayoutWidth - aiBarContentWidth)
            : 0
        implicitWidth: appletItem ? aiBarLayoutWidth : 0
        implicitHeight: appletItem ? appletItem.implicitHeight : 0
        rightPadding: aiBarRightPadding

        transform: Translate {
            x: delegateRoot.aiBarHorizontalOffset
        }

        contentItem: appletItem
        background: AppletItemBackground {
            x: Math.round(!delegateRoot.isAiBarApplet || (Panel.rootObject && Panel.rootObject.useColumnLayout)
                          ? (delegateRoot.width - width) / 2
                          : delegateRoot.hoverInset * -1)
            y: Math.round(!delegateRoot.isAiBarApplet || !(Panel.rootObject && Panel.rootObject.useColumnLayout)
                          ? (delegateRoot.height - height) / 2
                          : delegateRoot.hoverInset * -1)
            width: delegateRoot.unifiedHoverBackgroundWidth
            height: delegateRoot.unifiedHoverBackgroundHeight
            radius: height / 5
            enabled: false
            visible: delegateRoot.useUnifiedDockHoverBackground
            opacity: delegateHoverHandler.hovered ? 1 : 0
            D.ColorSelector.hovered: delegateHoverHandler.hovered

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }

        HoverHandler {
            id: delegateHoverHandler
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
            enabled: delegateRoot.useUnifiedDockHoverBackground && delegateRoot.visible
        }

        Timer {
            id: syncAppletItemTimer
            interval: 0
            repeat: false
            onTriggered: delegateRoot.syncAppletItem()
        }

        function scheduleSyncAppletItem() {
            if (appletItem) {
                syncAppletItemTimer.restart()
            }
        }

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

        onAppletItemChanged: {
            syncAppletItem()
            scheduleSyncAppletItem()
        }
        onVisibleChanged: scheduleSyncAppletItem()
        onWindowChanged: scheduleSyncAppletItem()
        onParentChanged: scheduleSyncAppletItem()

        Component.onCompleted: syncAppletItem()

        Component.onDestruction: {
            if (attachedAppletItem && attachedAppletItem.parent === delegateRoot) {
                attachedAppletItem.parent = null
            }
            attachedAppletItem = null
        }

        Connections {
            target: Panel

            function onHideStateChanged() {
                if (Panel.hideState !== Dock.Hide) {
                    delegateRoot.scheduleSyncAppletItem()
                }
            }
        }
    }
}
