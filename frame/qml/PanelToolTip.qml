// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control
    visible: false
    default property alias toolTipContent: toolTip.contentChildren
    property alias text: toolTip.text
    Panel.toolTipWindow.width: control.width + toolTip.leftPadding + toolTip.rightPadding
    Panel.toolTipWindow.height: control.height + toolTip.topPadding + toolTip.bottomPadding
    onVisibleChanged: {
        if (visible) {
            open()
        } else {
            close()
        }
    }

    function open()
    {
        var window = Panel.toolTipWindow
        if (!window)
            return

        window.xOffset = control.x
        window.yOffset = control.y
        window.show()
        toolTip.open()
    }
    function close()
    {
        var window = Panel.toolTipWindow
        if (!window)
            return

        window.close()
        toolTip.close()
    }
    function hide()
    {
        close()
    }

    ToolTip {
        id: toolTip
        padding: 0
        width: control.width
        height: control.height
        parent: Panel.toolTipWindow ? Panel.toolTipWindow.contentItem : undefined
        onParentChanged: function() {
            var window = Panel.toolTipWindow
            if (!window)
                return
            window.visibleChanged.connect(function() {
                if (!Panel.toolTipWindow.visible)
                    toolTip.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
}
