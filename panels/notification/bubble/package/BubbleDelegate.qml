// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: delegateRoot
    width: 360
    property var bubble: model
    property int maxCount: 3
    // ListView 的 remove 动画执行的时候,remove Item的index会以负数的方式出现
    property int realIndex: index < 0 ? ListView.view.count + index : index;
    
    height: bubbleContent.height
    z: -realIndex
    Bubble {
        id: bubbleContent
        width: 360
        bubble: delegateRoot.bubble
        
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
