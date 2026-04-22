// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0
import org.deepin.dtk 1.0

Control {
    id: root
    property bool useColumnLayout: false
    required property var shellSurface
    property int itemMargins
    property bool isOpened
    signal clicked()
    property bool contentHovered
    ColorSelector.hovered: root.contentHovered || root.hovered
    ColorSelector.pressed: mouseHandler.pressed
    property Palette textColor: DockPalette.iconTextPalette
    palette.windowText: ColorSelector.textColor

    onIsOpenedChanged: {
        if (root.isOpened) {
            toolTip.close()
        }
    }

    PanelToolTip {
        id: toolTip
        text: qsTr("Quick actions")
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    function showToolTipNow() {
        var point = quickpanelPlaceholder.mapToItem(null, quickpanelPlaceholder.width / 2, quickpanelPlaceholder.height / 2)
        toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
        toolTip.open()
    }

    contentItem: Grid {
        rows: root.useColumnLayout ? 2 : 1
        spacing: 0
        padding: 0

        Loader {
            id: placeholder
            Layout.alignment: Qt.AlignCenter
            active: root.shellSurface
            visible: active
            sourceComponent: TrayItemSurface {
                shellSurface: root.shellSurface
                onHoveredChanged: function () {
                    root.contentHovered = hovered
                }
            }
        }
        Control {
            id: quickpanelPlaceholder
            Layout.alignment: Qt.AlignCenter
            padding: itemMargins
            contentItem: DciIcon {
                sourceSize: Qt.size(16, 16)
                name: "dock-control-panel"
                palette: DTK.makeIconPalette(root.palette)
                theme: root.ColorSelector.controlTheme
                smooth: false
            }
            HoverHandler {
                onHoveredChanged: function () {
                    root.contentHovered = hovered
                    if (hovered && !root.isOpened) {
                        root.showToolTipNow()
                    } else {
                        toolTip.close()
                    }
                }
            }
        }
    }
    background: AppletItemBackground {
        isActive: root.isOpened
    }

    component TrayItemSurface: Item {
        implicitWidth: surfaceLayer.width
        implicitHeight: surfaceLayer.height
        property alias shellSurface: surfaceLayer.shellSurface
        property alias hovered: surfaceLayer.hovered

        ShellSurfaceItemProxy {
            id: surfaceLayer
            onWidthChanged: {
                if (!shellSurface || !(shellSurface.updatePluginGeometry))
                    return
                shellSurface.margins = root.itemMargins
                shellSurface.updatePluginGeometry(Qt.rect(Math.round(itemScenePoint.x),
                                                          Math.round(itemScenePoint.y),
                                                          Math.round(width),
                                                          Math.round(height)))
            }
            onHeightChanged: {
                if (!shellSurface || !(shellSurface.updatePluginGeometry))
                    return
                shellSurface.margins = root.itemMargins
                shellSurface.updatePluginGeometry(Qt.rect(Math.round(itemScenePoint.x),
                                                          Math.round(itemScenePoint.y),
                                                          Math.round(width),
                                                          Math.round(height)))
            }

            function localItemPoint() {
                let current = surfaceLayer
                let x = 0
                let y = 0
                while (current.parent) {
                    x += current.x
                    y += current.y
                    current = current.parent
                }

                return Qt.point(x, y)
            }

            property var itemScenePoint: {
                Panel.frontendWindowRect
                surfaceLayer.localItemPoint()
                return surfaceLayer.mapToItem(null, 0, 0)
            }

            onItemScenePointChanged: {
                if (!shellSurface || !(shellSurface.updatePluginGeometry))
                    return
                shellSurface.margins = root.itemMargins
                shellSurface.updatePluginGeometry(Qt.rect(Math.round(itemScenePoint.x),
                                                          Math.round(itemScenePoint.y),
                                                          Math.round(width),
                                                          Math.round(height)))
            }
        }
    }

    MouseArea {
        id: mouseHandler
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
