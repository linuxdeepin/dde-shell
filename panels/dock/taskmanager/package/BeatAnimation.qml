// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

SequentialAnimation {
    property Item target: parent
    id: root
    property real baseScale

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale
        to: baseScale
        duration: 880
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale
        to: baseScale * 1.2
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.2
        to: baseScale * 1.08
        duration: 80
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.08
        to: baseScale * 1.16
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.16
        to: baseScale * 1.1
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.1
        to: baseScale * 1.15
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.15
        to: baseScale * 1.12
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.12
        to: baseScale * 0.9
        duration: 160
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 0.9
        to: baseScale * 1.05
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.05
        to: baseScale * 0.95
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 0.95
        to: baseScale * 1
        duration: 120
    }

    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1
        to: baseScale * 1
        duration: 480
    }

    onStopped: root.target.scale = baseScale
    onFinished: loops = Animation.Infinite
}
