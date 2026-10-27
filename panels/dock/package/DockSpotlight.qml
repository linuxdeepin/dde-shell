// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

Item {
    id: spotlight
    property bool containsMouse: false
    property point cursorPosition: Qt.point(0, 0)
    property real itemSize: 48
    property bool darkTheme: false
    readonly property real baseRadius: Math.max(0, Math.min(itemSize * 3.0, height * 3.0, width * 0.18))
    readonly property real coreRadius: baseRadius * 0.35

    clip: true
    enabled: false
    visible: opacity > 0
    opacity: containsMouse ? 1 : 0

    Behavior on opacity {
        NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
    }

    // Move a single transform instead of evaluating both images' coordinates
    // on every pointer event. Both layers share the same cached 256px texture.
    Item {
        x: Math.max(0, Math.min(spotlight.cursorPosition.x, spotlight.width))
        y: Math.max(0, Math.min(spotlight.cursorPosition.y, spotlight.height))

        Image {
            objectName: "spotlightImage"
            width: spotlight.baseRadius * 2
            height: spotlight.baseRadius * 2.6
            x: -width / 2
            y: -height / 2
            source: "SpotlightGradient.png"
            fillMode: Image.Stretch
            smooth: true
            opacity: spotlight.darkTheme ? 0.14 : 0.2
        }

        Image {
            objectName: "spotlightCoreImage"
            width: spotlight.coreRadius * 2
            height: width
            x: -width / 2
            y: -height / 2
            source: "SpotlightGradient.png"
            fillMode: Image.Stretch
            smooth: true
            opacity: spotlight.darkTheme ? 0.23 : 0.32
        }
    }
}
