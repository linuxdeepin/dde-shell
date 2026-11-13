// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.ds 1.0

// dock 面板内统一的 tooltip。wayland（treeland）下交给合成器绘制：dock preview
// context 仍是同面板 taskmanager 持有的那一个，但每个 tooltip（即每个本组件的
// 实例）在合成器侧各有独立的 surface，切换条目时合成器才能在两个 tooltip 之间
// 播放过渡动画；协议不可用、非 wayland 或不在 dock 面板内时回退到
// PanelToolTipWindow。接口与 PanelToolTip 保持一致，调用方仍需在 open() 前设置
// DockPanelPositioner.bounding。
Item {
    id: control

    property string text
    property int toolTipX: 0
    property int toolTipY: 0
    // 供无障碍测试定位 tooltip 内容
    property string accessibleId

    width: impl.width
    height: impl.height

    readonly property bool useCompositorTooltip: !!Panel && Qt.platform.pluginName === "wayland"
    readonly property bool useColumnLayout: Panel ? Panel.position % 2 !== 0 : false
    // 当前 tooltip 是否由合成器绘制，决定 close() 走哪条路径
    property bool compositorTooltipActive: false

    readonly property bool toolTipVisible: compositorTooltipActive || impl.toolTipVisible

    function open() {
        if (useCompositorTooltip) {
            if (impl.toolTipVisible)
                impl.close()
            if (showOnCompositor()) {
                compositorTooltipActive = true
                return
            }
        }

        // 合成器协议不可用时回退，避免 tooltip 直接消失
        compositorTooltipActive = false
        impl.open()
    }

    function close() {
        if (compositorTooltipActive) {
            compositorTooltipActive = false
            Panel.hideDockTooltip(control)
            return
        }
        impl.close()
    }

    function hide() {
        close()
    }

    function showOnCompositor() {
        const anchor = DockPanelPositioner.bounding
        let xOffset, yOffset
        const interval = 10
        if (!useColumnLayout) {
            xOffset = anchor.x
            yOffset = Panel.position === Dock.Bottom ? -interval : interval + Panel.dockSize
        } else {
            xOffset = Panel.position === Dock.Right ? -interval : interval + Panel.dockSize
            yOffset = anchor.y
        }

        return Panel.requestDockTooltip(control, text, xOffset, yOffset, Panel.position)
    }

    onAccessibleIdChanged: {
        if (impl)
            impl.Accessible.id = accessibleId
    }

    PanelToolTip {
        id: impl
        Component.onCompleted: Accessible.id = control.accessibleId
        Accessible.role: Accessible.ToolTip
        text: control.text
        toolTipX: control.toolTipX
        toolTipY: control.toolTipY
    }
}
