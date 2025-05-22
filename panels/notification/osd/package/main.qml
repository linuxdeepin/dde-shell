// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS

Window {
    id: root
    visible: Applet.visible
    property var windowRadius: isSingleView ? 30 : D.DTK.platformTheme.windowRadius
    D.DWindow.windowRadius: root.windowRadius
    D.DWindow.enableBlurWindow: true
    D.DWindow.enabled: true
    D.DWindow.shadowOffset: Qt.point(0, 8)
    D.DWindow.shadowColor: D.DTK.themeType === D.ApplicationHelper.DarkType ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.1)
    D.DWindow.borderColor: D.DTK.themeType === D.ApplicationHelper.DarkType ? Qt.rgba(0, 0, 0, 0.7) : Qt.rgba(0, 0, 0, 0.1)
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

    D.StyledBehindWindowBlur {
        control: parent
        anchors.fill: parent
        blendColor: {
            if (valid) {
                return DS.Style.control.selectColor(undefined,
                                                    Qt.rgba(247 / 255.0, 247 / 255.0, 247 / 255.0, 0.4),
                                                    Qt.rgba(20 / 255, 20 / 255, 20 / 255, 0.6))
            }
            return DS.Style.control.selectColor(undefined,
                                                DS.Style.behindWindowBlur.lightNoBlurColor,
                                                DS.Style.behindWindowBlur.darkNoBlurColor)
        }
    }

    D.InsideBoxBorder {
        property D.Palette insideBorderColor: D.Palette {
            normal: Qt.rgba(1, 1, 1, 0.3)
            normalDark: Qt.rgba(1, 1, 1, 0.1)
        }
        radius: root.windowRadius
        anchors.fill: parent
        z: D.DTK.AboveOrder
        color: D.ColorSelector.insideBorderColor
    }

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
                }
            }
        }
    }
}
