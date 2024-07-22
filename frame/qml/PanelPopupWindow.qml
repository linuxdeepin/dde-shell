// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle

Window {
    id: root

    property real xOffset: 0
    property real yOffset: 0
    property Item currentItem
    x: selectValue(transientParent ? transientParent.x + xOffset : 0, Screen.virtualX + 10, Screen.virtualX + Screen.width - root.width - 10)
    y: selectValue(transientParent ? transientParent.y + yOffset : 0, Screen.virtualY + 10, Screen.virtualY + Screen.height - root.height - 10)
    function selectValue(value, min, max) {
        // wayland do not need to be limitted in the screen, this has been done by compositor
        if (Qt.platform.pluginName === "wayland")
            return value

        if (value < min)
            return min
        if (value > max)
            return max

        return value
    }
    // following transientParent's screen.
    Binding {
        when: root.transientParent
        target: root; property: "screen"
        value: root.transientParent ? root.transientParent.screen: undefined
    }
    // TODO: it's a qt bug which make Qt.Popup can not get input focus
    flags: Qt.platform.pluginName === "xcb" ? Qt.Tool : Qt.ToolTip
    D.DWindow.enabled: true
    D.DWindow.windowRadius: D.DTK.platformTheme.windowRadius < 0 ? 4 : D.DTK.platformTheme.windowRadius
    D.DWindow.enableSystemResize: false
    D.DWindow.enableBlurWindow: true
    // TODO set shadowOffset maunally.
    D.DWindow.shadowOffset: Qt.point(0, 10)
    D.ColorSelector.family: D.Palette.CrystalColor

    color: "transparent"
    onVisibleChanged: function (arg) {
        if (!arg)
            DS.closeChildrenWindows(root)
    }

    D.StyledBehindWindowBlur {
        control: parent
        anchors.fill: parent
        function blendColorAlpha(fallback) {
            var appearance = DS.applet("org.deepin.ds.dde-appearance")
            if (!appearance || appearance.opacity < 0)
                return fallback
            return appearance.opacity
        }
        blendColor: {
            if (valid) {
                return DStyle.Style.control.selectColor(undefined,
                                                    Qt.rgba(235 / 255.0, 235 / 255.0, 235 / 255.0, blendColorAlpha(0.6)),
                                                    Qt.rgba(0, 0, 0, blendColorAlpha(85 / 255)))
            }
            return DStyle.Style.control.selectColor(undefined,
                                                DStyle.Style.behindWindowBlur.lightNoBlurColor,
                                                DStyle.Style.behindWindowBlur.darkNoBlurColor)
        }
    }
}
