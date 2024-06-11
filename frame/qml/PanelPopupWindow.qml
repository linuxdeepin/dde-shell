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
    x: selectValue(transientParent.x + xOffset, 0, Screen.width - root.width)
    y: selectValue(transientParent.y + yOffset, 0, Screen.height - root.height)
    function selectValue(value, min, max) {
        if (value < min)
            return min
        if (value > max)
            return max

        return value
    }
    flags: Qt.Popup
    D.DWindow.enabled: true
    D.DWindow.windowRadius: 4 * Screen.devicePixelRatio
    D.DWindow.enableBlurWindow: true
    D.DWindow.shadowRadius: 8
    // TODO set shadowOffset maunally.
    D.DWindow.shadowOffset: Qt.point(0, 10)
    D.ColorSelector.family: D.Palette.CrystalColor

    color: "transparent"
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
