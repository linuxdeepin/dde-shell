// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml

import org.deepin.ds 1.0
import org.deepin.dtk 1.0

Control {
    id: root
    required property var shellSurface
    property bool isOpened
    signal clicked()
    property bool contentHovered
    padding: 5
    ColorSelector.hovered: root.contentHovered || root.hovered || root.isOpened

    PanelToolTip {
        id: toolTip
        text: qsTr("Quick actions")
    }

    contentItem: RowLayout {
        spacing: 5

        Loader {
            active: root.shellSurface
            visible: active
            sourceComponent: TrayItemSurface {
                shellSurface: root.shellSurface
                onHoveredChanged: function () {
                    root.contentHovered = hovered
                }
            }
        }
        DciIcon {
            id: quickpanelPlaceholder
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            name: "dock-control-panel"
            palette: DTK.makeIconPalette(root.palette)
            theme: root.ColorSelector.controlTheme
            sourceSize: Qt.size(quickpanelPlaceholder.width, quickpanelPlaceholder.width)
            HoverHandler {
                enabled: !root.isOpened
                onHoveredChanged: function () {
                    root.contentHovered = hovered
                    if (hovered) {
                        var point = quickpanelPlaceholder.mapToItem(null, quickpanelPlaceholder.width / 2, 0)
                        toolTip.toolTipX = Qt.binding(function () {
                            return point.x - toolTip.width / 2
                        })
                        toolTip.toolTipY = Qt.binding(function () {
                            return -toolTip.height - 10
                        })
                        toolTip.open()
                    } else {
                        toolTip.close()
                    }
                }
            }
        }
    }
    background: BoxPanel {
        radius: 4
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

        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.clicked()
            }
        }
    }

    component TrayItemSurface: Item {
        implicitWidth: surfaceLayer.width
        implicitHeight: surfaceLayer.height
        property alias shellSurface: surfaceLayer.shellSurface
        property alias hovered: hoverHandler.hovered

        HoverHandler {
            id: hoverHandler
            parent: surfaceLayer
        }

        ShellSurfaceItemProxy {
            id: surfaceLayer
            property var itemGlobalPoint: {
                var a = surfaceLayer
                var x = 0, y = 0
                while(a.parent) {
                    x += a.x
                    y += a.y
                    a = a.parent
                }

                return Qt.point(x, y)
            }

            onItemGlobalPointChanged: {
                if (!shellSurface || !(shellSurface.updatePluginGeometry))
                    return
                shellSurface.updatePluginGeometry(Qt.rect(itemGlobalPoint.x, itemGlobalPoint.y, surfaceLayer.width, surfaceLayer.height))
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.clicked()
        }
    }
}
