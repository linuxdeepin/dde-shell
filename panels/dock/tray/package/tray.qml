// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    readonly property string quickpanelTrayItemPluginId: "sound"
    readonly property var filterTrayPlugins: [quickpanelTrayItemPluginId]
    property bool quickPanelIsOpened: false

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : trayContainter.implicitWidth + nextAppletSpacing
    implicitHeight: useColumnLayout ? trayContainter.implicitHeight + nextAppletSpacing: Panel.rootObject.dockSize
    Component.onCompleted: {
        Applet.trayPluginModel = Qt.binding(function () {
            return DockCompositor.trayPluginSurfaces
        })
    }

    PanelPopup {
        id: popup
        property alias shellSurface: popupContent.shellSurface
        width: popupContent.width
        height: popupContent.height
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y

        Item {
            anchors.fill: parent
            ShellSurfaceItemProxy {
                id: popupContent
                anchors.centerIn: parent
                autoClose: true
                onSurfaceDestroyed: function () {
                    popup.close()
                }
            }
        }
    }

    PanelMenu {
        id: popupMenu
        property alias shellSurface: popupMenuContent.shellSurface
        width: popupMenuContent.width
        height: popupMenuContent.height
        menuX: DockPositioner.x
        menuY: DockPositioner.y

        Item {
            anchors.fill: parent
            ShellSurfaceItemProxy {
                id: popupMenuContent
                anchors.centerIn: parent
                autoClose: true
                onSurfaceDestroyed: function () {
                    popupMenu.close()
                }
            }
        }
    }

    PanelToolTip {
        id: toolTip
        property alias shellSurface: toolTipContent.shellSurface
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y

        ShellSurfaceItemProxy {
            id: toolTipContent
            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: function () {
                toolTip.close()
            }
        }
    }

    PanelPopup {
        id: stashedPopup
        width: stashedContainer.width
        height: stashedContainer.height

        Control {
            id: stashedContainer
            padding: 10
            contentItem: StashContainer {
                id: stashContainer
                color: "transparent"
                model: DDT.SortFilterProxyModel {
                    sourceModel: DDT.TraySortOrderModel
                    filterRowCallback: (sourceRow, sourceParent) => {
                        let index = sourceModel.index(sourceRow, 0, sourceParent)
                        return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) === "stashed" &&
                               sourceModel.data(index, DDT.TraySortOrderModel.VisibilityRole) === true
                    }
                }
                anchors.centerIn: parent
            }
        }
    }
    Connections {
        target: DDT.TraySortOrderModel
        function onActionsAlwaysVisibleChanged(val) {
            if (val) {
                if (!stashedPopup.visible) {
                    // TODO: position?
                    stashedPopup.open()
                }
            }
        }
    }

    TrayContainer {
        id: trayContainter
        isHorizontal: !tray.useColumnLayout
        model: DDT.TraySortOrderModel
        collapsed: DDT.TraySortOrderModel.collapsed
        trayHeight: isHorizontal ? tray.implicitHeight : tray.implicitWidth
        color: "transparent"
        Component.onCompleted: {
            DDT.TrayItemPositionManager.layoutHealthCheck(1500)
        }
    }

    function isQuickPanelPopup(popupSurface)
    {
        return popupSurface &&
                popupSurface.pluginId === tray.quickpanelTrayItemPluginId
    }
    onQuickPanelIsOpenedChanged: function ()
    {
        if (tray.quickPanelIsOpened &&
                toolTip.toolTipVisible &&
                isQuickPanelPopup(toolTip.shellSurface)) {
            toolTip.close()
        }
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            if (!isTrayPluginPopup(popupSurface))
                return

            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                if (tray.quickPanelIsOpened && isQuickPanelPopup(popupSurface)) {
                    // don't show the surface, and release it.
                    DockCompositor.closeShellSurface(popupSurface)
                    return
                }

                console.log("tray's tooltip created", popupSurface.pluginId, popupSurface.itemKey)
                toolTip.shellSurface = popupSurface
                toolTip.DockPanelPositioner.bounding = Qt.binding(function () {
                    var point = Qt.point(toolTip.shellSurface.x, toolTip.shellSurface.y)
                    return Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                })
                toolTip.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypePanel) {
                console.log("tray's popup created", popupSurface.pluginId, popupSurface.itemKey)
                popup.shellSurface = popupSurface
                popup.DockPanelPositioner.bounding = Qt.binding(function () {
                    var point = Qt.point(popup.shellSurface.x, popup.shellSurface.y)
                    return Qt.rect(point.x, point.y, popup.width, popup.height)
                })
                popup.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypeMenu) {
                console.log("tray's menu created", popupSurface.pluginId, popupSurface.itemKey)
                popupMenu.shellSurface = popupSurface
                popupMenu.DockPositioner.bounding = Qt.binding(function () {
                    var point = Qt.point(popupMenu.shellSurface.x, popupMenu.shellSurface.y)
                    return Qt.rect(point.x, point.y, popupMenu.width, popupMenu.height)
                })
                popupMenu.open()
            }
        }
        function isTrayPluginPopup(popupSurface)
        {
            let surfaceId = `${popupSurface.pluginId}::${popupSurface.itemKey}`
            if (stashContainer.isStashPopup(surfaceId))
                return false
            if (DockCompositor.findSurfaceFromModel(DockCompositor.trayPluginSurfaces, surfaceId))
                return true
            if (DockCompositor.findSurfaceFromModel(DockCompositor.fixedPluginSurfaces, surfaceId))
                return true
            return false
        }
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

                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin", "sectionType": preferredSection, "forbiddenSections": forbiddenSections})
            }
            // actually only for datetime plugin currently
            for (let i = 0; i < DockCompositor.fixedPluginSurfaces.count; i++) {
                let item = DockCompositor.fixedPluginSurfaces.get(i).shellSurface
                let surfaceId = `${item.pluginId}::${item.itemKey}`
                let forbiddenSections = ["stashed", "collapsable", "pinned"]
                let preferredSection = "fixed"

                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin", "sectionType": preferredSection, "forbiddenSections": forbiddenSections})
            }
            DDT.TraySortOrderModel.availableSurfaces = surfacesData
            console.log("onPluginSurfacesUpdated", surfacesData.length)
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
