// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0

Item {
    id: delegateRoot
    width: ListView.view ? ListView.view.width : 360
    // Close button overhang: half of 20px button height minus 2px visual offset
    // (see NotifyItemContent.qml closePlaceHolder topMargin: -height / 2 + 2)
    readonly property int closeButtonOverhang: 8
    property var bubble: model
    property int maxCount: 3
    // ListView 的 remove 动画执行的时候,remove Item的index会以负数的方式出现
    property int realIndex: index < 0 ? ListView.view.count + index : index;
    
    height: bubbleContent.height
    z: -realIndex

    HoverHandler {
        id: delegateHoverHandler
        margin: delegateRoot.closeButtonOverhang
        onHoveredChanged: {
            Applet.setHoveredId(hovered && delegateRoot.bubble ? delegateRoot.bubble.id : 0)
        }
    }

    Bubble {
        id: bubbleContent
        width: 360
        anchors.right: parent.right
        anchors.rightMargin: 10
        bubble: delegateRoot.bubble
        parentHovered: delegateHoverHandler.hovered
        
        transformOrigin: Item.Top
        
        y: {
            // normal bubble dont need to move
            if (realIndex < delegateRoot.maxCount) 
                return 0

            let spacing = 10
            let peekAmount = 8
            // 根据 realIndex 计算出超出部分的折叠层数（最多折叠3层，再多层保留为了动画淡出）
            let levelsFolded = Math.min(realIndex - (delegateRoot.maxCount - 1), 3)
            let ret = levelsFolded * (delegateRoot.height + spacing - peekAmount)
            return ret
        }

        scale: {
            if (realIndex < delegateRoot.maxCount)
                return 1.0

            let levelsFolded = Math.min(realIndex - (delegateRoot.maxCount - 1), 3)
            return 1.0 - levelsFolded * 0.05
        }

        opacity: realIndex >= (delegateRoot.maxCount + 2) ? 0 : 1.0

        Behavior on y { NumberAnimation { duration: 600; easing.type: Easing.OutExpo } }
        Behavior on scale { NumberAnimation { duration: 600; easing.type: Easing.OutExpo } }
        Behavior on opacity { NumberAnimation { duration: 600; easing.type: Easing.OutExpo } }
    }
}
