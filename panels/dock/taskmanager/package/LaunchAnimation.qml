// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

SequentialAnimation {
    property Item target: parent
    id: root
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0)
        to: value(0.15)
        duration: 60
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.15)
        to: value(0.05)
        duration: 60
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.05)
        to: value(0.15)
        duration: 60
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.15)
        to: value(0.10)
        duration: 60
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.10)
        to: value(0.15)
        duration: 60
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.15)
        to: value(0.15)
        duration: 500
    }
    PropertyAnimation {
        target: root.target
        property: getAxis()
        from: value(0.15)
        to: value(0)
        duration: 60
    }
    onStarted: {
        target.anchors.centerIn = null
    }
    onStopped: {
        target.anchors.centerIn = target.parent
    }

    function getAxis() {
        if (Panel.position === Dock.Top || Panel.position === Dock.Bottom)
            return "y"
        else
            return "x"
    }

    function value(amplitude) {
        switch (Panel.position) {
        case Dock.Top:
            return target.y + target.height * amplitude
        case Dock.Bottom:
            return target.y - target.height * amplitude
        case Dock.Left:
            return target.x + target.width * amplitude
        case Dock.Right:
            return target.x - target.width * amplitude
        }
    }
}
