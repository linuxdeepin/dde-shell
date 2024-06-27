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
    property bool isOpened
    signal clicked()
    contentItem: RowLayout {
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
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            sourceComponent: Item {
                ShellSurfaceItemProxy {
                    anchors.fill: parent
                    shellSurface: root.shellSurface
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.clicked()
                }
            }
        }

        DciIcon {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
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
}
