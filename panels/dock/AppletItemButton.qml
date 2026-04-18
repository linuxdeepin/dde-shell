// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk

IconButton {
    id: control
    property bool isActive
    property real radius: 4
    property point lastSpotlightPoint: Qt.point(0, 0)
    property bool autoClosePopup: false

    padding: 4
    topPadding: undefined
    bottomPadding: undefined
    leftPadding: undefined
    rightPadding: undefined

    textColor: DockPalette.iconTextPalette
    display: IconLabel.IconOnly

    icon.width: 16
    icon.height: 16

    function mapSpotlightPoint(localPoint) {
        const point = localPoint || Qt.point(width / 2, height / 2)
        return mapToItem(null, point.x, point.y)
    }

    function updateSpotlight(localPoint) {
        lastSpotlightPoint = mapSpotlightPoint(localPoint)
        Panel.reportMousePresence(true, lastSpotlightPoint)
    }

    function clearSpotlight() {
        Panel.reportMousePresence(false, lastSpotlightPoint)
    }

    Connections {
        target: control
        enabled: autoClosePopup
        function onClicked() {
            Panel.requestClosePopup()
        }
    }

    background: AppletItemBackground {
        radius: control.radius
        isActive: control.isActive
    }

    Component.onCompleted: {
        contentItem.smooth = false
    }

    HoverHandler {
        id: spotlightHoverHandler
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
        enabled: control.enabled && control.visible && control.hoverEnabled

        onPointChanged: {
            if (hovered) {
                spotlightClearTimer.stop()
                control.updateSpotlight(spotlightHoverHandler.point.position)
            }
        }

        onHoveredChanged: {
            if (hovered) {
                spotlightClearTimer.stop()
                control.updateSpotlight()
                return
            }

            spotlightClearTimer.restart()
        }
    }

    Timer {
        id: spotlightClearTimer
        interval: 70
        repeat: false
        onTriggered: {
            if (!spotlightHoverHandler.hovered) {
                control.clearSpotlight()
            }
        }
    }
}
