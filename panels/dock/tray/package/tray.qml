// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0
import org.deepin.ds.dock.tray 1.0 as DDT

AppletItem {
    id: tray

    readonly property int nextAppletSpacing: 6
    property bool useColumnLayout: Panel.rootObject.positionForAnimation % 2
    property int dockOrder: 25
    readonly property string quickpanelTrayItemPluginId: "sound"
    readonly property var filterTrayPlugins: [quickpanelTrayItemPluginId]

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : trayContainter.implicitWidth + nextAppletSpacing
    implicitHeight: useColumnLayout ? trayContainter.implicitHeight + nextAppletSpacing: Panel.rootObject.dockSize
    Component.onCompleted: {
        Applet.trayPluginModel = Qt.binding(function () {
            return DockCompositor.trayPluginSurfaces
        })
        Applet.quickPluginModel = Qt.binding(function () {
            return DockCompositor.quickPluginSurfaces
        })
        Applet.fixedPluginModel = Qt.binding(function () {
            return DockCompositor.fixedPluginSurfaces
        })
    }

    PanelPopup {
        id: stashedPopup
        width: stashedContainer.width
        height: stashedContainer.height

        property alias dropHover: stashContainer.dropHover
        property alias stashItemDragging: stashContainer.stashItemDragging

        property bool wasOpenBeforeDrag: false
        // 拖拽成功标记（图标是否成功移入 stash），由放下的回调根据返回值设置
        property bool stashDropSucceeded: false

        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y

        property point collapsedBtnCenterPoint: Qt.point(0, 0)

        Control {
            id: stashedContainer
            padding: 10
            contentItem: StashContainer {
                id: stashContainer
                color: "transparent"
                model: DDT.SortFilterProxyModel {
                    sourceModel: DDT.TraySortOrderModel
                    sortRoleName: "visualIndex"
                    sortOrder: Qt.AscendingOrder
                    filterRowCallback: (sourceRow, sourceParent) => {
                        let index = sourceModel.index(sourceRow, 0, sourceParent)
                        return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) === "stashed" &&
                               sourceModel.data(index, DDT.TraySortOrderModel.VisibilityRole) === true
                    }
                    sortRole: DDT.TraySortOrderModel.VisualIndexRole
                }
                anchors.centerIn: parent
                onRowCountChanged: {
                    if (stashContainer.rowCount === 0 || stashContainer.columnCount === 0) {
                        stashedPopup.close()
                    }
                }
                // 拖拽成功移入 stash 时由 StashContainer 回调
                onStashDropSucceeded: function() {
                    stashedPopup.stashDropSucceeded = true
                }
            }
        }

        Component.onCompleted: {
            DockPanelPositioner.bounding = Qt.binding(function () {
                return Qt.rect(collapsedBtnCenterPoint.x, collapsedBtnCenterPoint.y, stashedPopup.width, stashedPopup.height)
            })
        }
    }
    Connections {
        target: DDT.TraySortOrderModel
        function onActionsAlwaysVisibleChanged(val) {
            if (!val && !Panel.contextDragging && !stashedPopup.dropHover) {
                closeStashPopupTimer.start()
            }
        }
    }

    // Bug to prevent icons from returning to the application tray when the tray is already hidden, which can cause layout confusion
    Timer {
        id: closeStashPopupTimer
        interval: 10
        repeat: false
        onTriggered: {
            // 拖拽仍在进行中时，绝不关闭（可能正要拖入面板）
            if (Panel.contextDragging)
                return
            // 只在“拖拽前面板未打开”且“拖拽未成功移入 stash”时才关闭
            if (!stashedPopup.dropHover
                    && !stashedPopup.wasOpenBeforeDrag
                    && !stashedPopup.stashDropSucceeded) {
                stashedPopup.close()
            }
        }
    }

    Connections {
        target: Panel
        function onContextDraggingChanged() {
            // 拖拽开始时记录面板状态，拖拽结束后据此决定是否自动关闭
            if (Panel.contextDragging) {
                stashedPopup.wasOpenBeforeDrag = stashedPopup.popupVisible
                stashedPopup.stashDropSucceeded = false
            }
        }
    }


    TrayContainer {
        id: trayContainter
        isHorizontal: !tray.useColumnLayout
        model: DDT.TraySortOrderModel
        collapsed: DDT.TraySortOrderModel.collapsed
        trayHeight: Panel.rootObject.dockSize
        surfaceAcceptor: isTrayPluginPopup
        color: "transparent"
        Component.onCompleted: {
            DDT.TrayItemPositionManager.layoutHealthCheck(1500)
        }
        // 拖拽成功移入 stash 时由 TrayContainer 回调（命中 action-show-stash）
        onStashDropSucceeded: function() {
            stashedPopup.stashDropSucceeded = true
        }
    }

    function isTrayPluginPopup(surfaceId) {
        if (stashContainer.isStashPopup(surfaceId))
            return false
        if (DockCompositor.findSurfaceFromModel(DockCompositor.trayPluginSurfaces, surfaceId))
            return true
        if (DockCompositor.findSurfaceFromModel(DockCompositor.fixedPluginSurfaces, surfaceId))
            return true
        return false
    }

    Connections {
        target: DockCompositor
        function onPluginSurfacesUpdated() {
            let surfacesData = []
            for (let i = 0; i < DockCompositor.trayPluginSurfaces.count; i++) {
                let item = DockCompositor.trayPluginSurfaces.get(i).shellSurface
                if (filterTrayPlugins.indexOf(item.pluginId) >= 0)
                    continue;
                let surfaceId = `${item.pluginId}::${item.itemKey}`
                let forbiddenSections = ["fixed"]
                let preferredSection = item.pluginId === "application-tray" ? "stashed" : "collapsable"

                if (item.pluginSizePolicy === Dock.Custom) {
                    forbiddenSections = ["stashed", "fixed"]
                    preferredSection = "pinned"
                }

                if (item.pluginFlags & 0x1000) { // force dock.
                    forbiddenSections = ["stashed", "collapsable", "fixed"]
                    preferredSection = "pinned"
                }

                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin", "sectionType": preferredSection, "forbiddenSections": forbiddenSections, "pluginFlags": item.pluginFlags})
            }
            // actually only for datetime plugin currently
            for (let i = 0; i < DockCompositor.fixedPluginSurfaces.count; i++) {
                let item = DockCompositor.fixedPluginSurfaces.get(i).shellSurface
                let surfaceId = `${item.pluginId}::${item.itemKey}`
                let forbiddenSections = ["stashed", "collapsable", "pinned"]
                let preferredSection = "fixed"

                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin", "sectionType": preferredSection, "forbiddenSections": forbiddenSections, "pluginFlags": item.pluginFlags})
            }
            DDT.TraySortOrderModel.availableSurfaces = surfacesData
            console.log("onPluginSurfacesUpdated", surfacesData.length)
            Applet.emitPluginsChanged()
        }

        function onRequestShutdown(type) {
            var shutdown = DS.applet("org.deepin.ds.dde-shutdown")
            if (shutdown) {
                shutdown.requestShutdown(type)
            } else {
                console.warn("shutdown applet not found")
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.rootObject
        sizeFollowsWindow: true
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.popupWindow
        sizeFollowsWindow: false
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.toolTipWindow
        sizeFollowsWindow: false
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.menuWindow
        sizeFollowsWindow: false
    }
}
