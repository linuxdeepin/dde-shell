// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.dtk 1.0
import org.deepin.dtk.style 1.0 as DStyle
import org.deepin.ds.notificationcenter

// TODO FloatingPanel
Control {
    id: control

    property Palette backgroundColor: DStyle.Style.floatingPanel.background
    property Palette backgroundNoBlurColor: DStyle.Style.floatingPanel.backgroundNoBlur
    property Palette dropShadowColor: DStyle.Style.floatingPanel.dropShadow
    property Palette outsideBorderColor: DStyle.Style.floatingPanel.outsideBorder
    property Palette insideBorderColor: DStyle.Style.floatingPanel.insideBorder
    // corner radius
    property int radius: DStyle.Style.floatingPanel.radius

    contentItem: Item {
        implicitWidth: DStyle.Style.floatingPanel.width
        implicitHeight: DStyle.Style.floatingPanel.height

        Loader {
            anchors.fill: backgroundRect
            active: control.dropShadowColor
            sourceComponent: BoxShadow {
                shadowOffsetX: 0
                shadowOffsetY: 6
                shadowColor: control.ColorSelector.dropShadowColor
                shadowBlur: 20
                cornerRadius: backgroundRect.radius
                spread: 0
                hollow: true
            }
        }

        Rectangle {
            id: backgroundRect
            anchors.fill: parent
            radius: control.radius
            color: control.ColorSelector.backgroundNoBlurColor
        }

        Loader {
            anchors.fill: backgroundRect
            active: control.insideBorderColor && control.ColorSelector.controlTheme === ApplicationHelper.DarkType
            sourceComponent: InsideBoxBorder {
                radius: backgroundRect.radius
                color: control.ColorSelector.insideBorderColor
                borderWidth: DStyle.Style.control.borderWidth
            }
        }

        Loader {
            anchors.fill: backgroundRect
            active: control.outsideBorderColor
            sourceComponent: OutsideBoxBorder {
                radius: backgroundRect.radius
                color: control.ColorSelector.outsideBorderColor
                borderWidth: DStyle.Style.control.borderWidth
            }
        }
    }

    background: BoundingRectangle {
        color: "transparent"
    }
}

