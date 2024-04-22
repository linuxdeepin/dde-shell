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
        to: baseScale * 1.4
        duration: 120
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.4
        to: baseScale * 1.3
        duration: 100
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.3
        to: baseScale * 1.4
        duration: 80
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.4
        to: baseScale * 1.3
        duration: 80
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.3
        to: baseScale * 1.3
        duration: 500
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.3
        to: baseScale
        duration: 100
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale
        to: baseScale * 1.1
        duration: 100
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale * 1.1
        to: baseScale
        duration: 100
    }
    PropertyAnimation {
        target: root.target
        property: "scale"
        from: baseScale
        to: baseScale
        duration: 1500
    }
    onStopped: root.target.scale = baseScale
    onFinished: loops = Animation.Infinite
}
