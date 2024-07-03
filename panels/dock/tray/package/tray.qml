// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT

AppletItem {
    id: tray

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    readonly property string quickpanelTrayItemPluginId: "sound"
    readonly property var filterTrayPlugins: [quickpanelTrayItemPluginId]
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : overflowId.implicitWidth
    implicitHeight: useColumnLayout ? overflowId.implicitHeight : Panel.rootObject.dockSize
    Component.onCompleted: {
        Applet.trayPluginModel = Qt.binding(function () {
            return DockCompositor.trayPluginSurfaces
        })
    }

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        Item {
            anchors.fill: parent
            ShellSurfaceItem {
                id: popupContent
                anchors.centerIn: parent
                onSurfaceDestroyed: function () {
                    popup.close()
                }
            }
        }
    }

    PanelToolTip {
        id: toolTip
        property alias shellSurface: toolTipContent.shellSurface

        ShellSurfaceItem {
            id: toolTipContent
            anchors.centerIn: parent
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
                color: "transparent"
                model: DDT.SortFilterProxyModel {
                    sourceModel: DDT.TraySortOrderModel
                    filterRowCallback: (sourceRow, sourceParent) => {
                        console.log(sourceRow, sourceParent)
                        let index = sourceModel.index(sourceRow, 0, sourceParent)
                        return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) === "stashed" &&
                               sourceModel.data(index, DDT.TraySortOrderModel.VisibilityRole) === true
                    }
                }
                anchors.centerIn: parent
            }
        }
    }

    GridLayout {
        id: overflowId
        columns: 1
        rows: 1
        anchors.centerIn: parent
        flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
        property bool trayVisible: true
        columnSpacing: 10
        rowSpacing: 10

        TrayContainer {
            isHorizontal: !tray.useColumnLayout
            model: DDT.TraySortOrderModel
            collapsed: DDT.TraySortOrderModel.collapsed
            trayHeight: isHorizontal ? tray.implicitHeight : tray.implicitWidth
            color: "transparent"
        }
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            if (!isTrayPluginPopup(popupSurface))
                return

            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                toolTip.shellSurface = popupSurface
                toolTip.toolTipX = Qt.binding(function () {
                    return toolTip.shellSurface.x - toolTip.width / 2
                })
                toolTip.toolTipY = Qt.binding(function () {
                    return -toolTip.height - 10
                })
                toolTip.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypeMenu) {
                popupContent.shellSurface = popupSurface
                popup.popupX = Qt.binding(function () {
                    return popupContent.shellSurface.x
                })
                popup.popupY = Qt.binding(function () {
                    return popupContent.shellSurface.y - popup.height
                })
                popup.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypePanel) {
                popupContent.shellSurface = popupSurface
                popup.popupX = Qt.binding(function () {
                    return popupContent.shellSurface.x - popup.width / 2
                })
                popup.popupY = Qt.binding(function () {
                    return -popup.height - 10
                })
                popup.open()
            }
        }
        function isTrayPluginPopup(popupSurface)
        {
            return DockCompositor.findSurface(`${popupSurface.pluginId}::${popupSurface.itemKey}`)
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
                let forbiddenSections = item.pluginSizePolicy === Dock.Custom ? ["stashed", "collapsable", "pinned"] : ["fixed"]
                let preferredSection = item.pluginSizePolicy === Dock.Custom ? "fixed" : "collapsable"
                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin", "sectionType": preferredSection, "forbiddenSections": forbiddenSections})
                console.log(surfaceId, item, item.pluginId, "surfaceId")
            }
            DDT.TraySortOrderModel.availableSurfaces = surfacesData
            console.log("onPluginSurfacesUpdated", surfacesData)
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
}
