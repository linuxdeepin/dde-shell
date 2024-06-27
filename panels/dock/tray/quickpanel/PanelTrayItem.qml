// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml

import org.deepin.dtk 1.0

Control {
    id: root
    required property var shellSurface
    required property var trayQuickPanelItemSurface
    property bool isOpened
    signal clicked()
    padding: 5
    contentItem: RowLayout {
        spacing: 5
        TapHandler {
            id: clickedLayer
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.clicked()
            }
        }

        Loader {
            active: root.shellSurface
            visible: active
            sourceComponent: TrayItemSurface {
                shellSurface: root.shellSurface
            }
        }
        Loader {
            id: quickpanelPlaceholder
            active: root.trayQuickPanelItemSurface
            visible: active
            sourceComponent: TrayItemSurface {
                shellSurface: root.trayQuickPanelItemSurface
            }
        }
        DciIcon {
            visible: !quickpanelPlaceholder.visible
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            name: "dock-control-panel"
        }
    }
    background: BoxPanel {
        radius: 0
        color2: color1
        property Palette openedPalette: Palette {
            normal {
                common: Qt.rgba(16.0 / 255, 16.0 / 255, 16.0 / 255, 0.2)
            }
        }
        property Palette unopenedPalette: Palette {
            normal {
                common: ("transparent")
            }
            hovered {
                crystal:  Qt.rgba(16.0 / 255, 16.0 / 255, 16.0 / 255, 0.2)
            }
        }
        color1: isOpened ? openedPalette : unopenedPalette
        insideBorderColor: null
        outsideBorderColor: null
    }

    component TrayItemSurface: Item {
        implicitWidth: surfaceLayer.width
        implicitHeight: surfaceLayer.height
        property alias shellSurface: surfaceLayer.shellSurface

        ShellSurfaceItemProxy {
            id: surfaceLayer
            property var itemGlobalPoint: surfaceLayer.mapToItem(null, 0, 0)
            onWidthChanged: updateSurface()
            onHeightChanged: updateSurface()
            onItemGlobalPointChanged: updateSurface()

            function updateSurface()
            {
                if (!shellSurface || !(shellSurface.updatePluginGeometry))
                    return

                var pos = surfaceLayer.mapToItem(null, 0, 0)
                shellSurface.updatePluginGeometry(Qt.rect(pos.x, pos.y, surfaceLayer.width, surfaceLayer.height))
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.clicked()
        }
    }
}
