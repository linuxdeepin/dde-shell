// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

Window {
    id: root
    visible: Applet.visible
    D.DWindow.windowRadius: isSingleView ? 30 : D.DTK.platformTheme.windowRadius
    D.DWindow.enableBlurWindow: true
    D.DWindow.enabled: true
    color: "transparent"
    DLayerShellWindow.bottomMargin: 180
    DLayerShellWindow.layer: DLayerShellWindow.LayerOverlay
    DLayerShellWindow.anchors: DLayerShellWindow.AnchorBottom
    palette: D.DTK.palette

    screen: Qt.application.screens[0]
    // TODO `Qt.application.screens[0]` maybe invalid, why screen is changed.
    onScreenChanged: {
        root.screen = Qt.binding(function () { return Qt.application.screens[0]})
    }

    width: osdView ? osdView.width : 100
    height: osdView ? osdView.height : 100

    property Item osdView
    property bool isSingleView: false

    Control {
        property D.Palette textColor: D.Palette {
            normal: Qt.rgba(0, 0, 0, 1)
            normalDark: Qt.rgba(1, 1, 1, 1)
        }
        palette.windowText:  D.ColorSelector.textColor

        Repeater {
            model: Applet.appletItems
            delegate: Loader {
                active: modelData.update(Applet.osdType)
                onActiveChanged: {
                    if (active) {
                        root.isSingleView = modelData.singleView
                        root.osdView = this
                    }
                }

                sourceComponent: Control {
                    contentItem: model.data
                    background: D.FloatingPanel {
                        implicitWidth:  100
                        implicitHeight: 40
                    }
                }
            }
        }
    }
}
