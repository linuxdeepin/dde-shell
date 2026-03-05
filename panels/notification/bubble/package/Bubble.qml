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
    property bool isRemoving: false
    
    Connections {
        target: Applet.bubbles
        function onBubbleAboutToRemove(id) {
            if (id === bubble.id) {
                control.isRemoving = true
            }
        }
    }
    
    onHoveredChanged: function () {
        if (control.hovered) {
            Applet.bubbles.delayRemovedBubble = bubble.id
        } else {
            Applet.bubbles.delayRemovedBubble = 0
        }
    }
    
    states: [
        State {
            name: "removing"
            when: control.isRemoving
            PropertyChanges {
                target: control
                x: control.width
                opacity: 0
            }
        }
    ]
    
    transitions: [
        Transition {
            to: "removing"
            NumberAnimation {
                properties: "x,opacity"
                duration: Applet.bubbles.removeAnimationDuration
                easing.type: Easing.InExpo
            }
        }
    ]

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
