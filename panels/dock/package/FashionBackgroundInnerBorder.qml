// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root

    property real cornerRadius: 0
    property real devicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    property color topColor: Qt.rgba(1, 1, 1, 0.3)
    property color bottomColor: Qt.rgba(1, 1, 1, 0.1)
    readonly property real borderWidth: 1 / Math.max(1, devicePixelRatio)

    enabled: false

    function clampRadius(radius, width, height) {
        return Math.max(0, Math.min(radius, Math.min(width, height) / 2))
    }

    function roundedRectPath(ctx, x, y, width, height, radius) {
        if (radius <= 0) {
            ctx.rect(x, y, width, height)
            return
        }

        const right = x + width
        const bottom = y + height
        ctx.moveTo(x + radius, y)
        ctx.lineTo(right - radius, y)
        ctx.arcTo(right, y, right, y + radius, radius)
        ctx.lineTo(right, bottom - radius)
        ctx.arcTo(right, bottom, right - radius, bottom, radius)
        ctx.lineTo(x + radius, bottom)
        ctx.arcTo(x, bottom, x, bottom - radius, radius)
        ctx.lineTo(x, y + radius)
        ctx.arcTo(x, y, x + radius, y, radius)
    }

    Canvas {
        id: borderCanvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            if (width <= 0 || height <= 0 || root.borderWidth <= 0) {
                return
            }

            const inset = root.borderWidth / 2
            const drawWidth = Math.max(0, width - root.borderWidth)
            const drawHeight = Math.max(0, height - root.borderWidth)
            const radius = root.clampRadius(root.cornerRadius - inset, drawWidth, drawHeight)

            const gradient = ctx.createLinearGradient(0, 0, 0, height)
            gradient.addColorStop(0, root.topColor)
            gradient.addColorStop(1, root.bottomColor)

            ctx.strokeStyle = gradient
            ctx.lineWidth = root.borderWidth
            ctx.beginPath()
            root.roundedRectPath(ctx, inset, inset, drawWidth, drawHeight, radius)
            ctx.stroke()
        }

        Connections {
            target: root

            function onCornerRadiusChanged() {
                borderCanvas.requestPaint()
            }

            function onDevicePixelRatioChanged() {
                borderCanvas.requestPaint()
            }

            function onTopColorChanged() {
                borderCanvas.requestPaint()
            }

            function onBottomColorChanged() {
                borderCanvas.requestPaint()
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Component.onCompleted: requestPaint()
    }
}
