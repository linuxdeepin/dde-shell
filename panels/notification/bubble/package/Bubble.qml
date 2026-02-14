// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

Control {
    id: control
    height: loader.height
    property var bubble
    onHoveredChanged: function () {
        if (control.hovered) {
            Applet.bubbles.delayRemovedBubble = bubble.id
        } else {
            Applet.bubbles.delayRemovedBubble = NotifyEntity.InvalidId
        }
    }

    Loader {
        id: loader
        width: control.width
        sourceComponent: bubble.level <= 1 ? normalCom : overlayCom
    }
    Component {
        id: normalCom
        NormalBubble {
            bubble: control.bubble
        }
    }
    Component {
        id: overlayCom
        OverlayBubble {
            bubble: control.bubble
        }
    }
}
