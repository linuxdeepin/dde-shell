// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

ActionButton {
    id: root

    property int radius: 16
    property bool forcusBorderVisible: root.visualFocus

    icon.width: 24
    icon.height: 24

    background: BoxPanel {
        radius: root.radius
        color1: Palette {
            normal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.5)
        }

        color2: color1
        insideBorderColor: Palette {
            normal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.2)
        }
        outsideBorderColor: Palette {
            normal: Qt.rgba(0, 0, 0, 0.08)
        }
        dropShadowColor: Palette {
            normal: Qt.rgba(0, 0, 0, 0.2)
        }
        innerShadowColor1: null
        innerShadowColor2: innerShadowColor1
        Loader {
            anchors.fill: parent
            active: root.forcusBorderVisible

            sourceComponent: FocusBoxBorder {
                radius: root.radius
                color: root.palette.highlight
            }
        }
    }
}
