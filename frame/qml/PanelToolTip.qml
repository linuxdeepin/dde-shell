// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control

    default property alias toolTipContent: toolTip.contentChildren
    property alias text: toolTip.text
    property alias toolTipVisible: toolTip.visible
    property int toolTipX: 0
    property int toolTipY: 0
    property int margins: 10
    onToolTipVisibleChanged: {
        if (toolTipVisible) {
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
        window.width = Qt.binding(function() {
            return toolTip.width + toolTip.leftPadding + toolTip.rightPadding
        })
        window.height = Qt.binding(function() {
            return toolTip.height + toolTip.topPadding + toolTip.bottomPadding
        })

        var pointX = control.toolTipX - window.width / 2
        var pointY = control.toolTipY - window.height - control.margins
        window.xOffset = pointX
        window.yOffset = pointY
        window.show()
        toolTip.open()
    }
    function close()
    {
        var window = Panel.toolTipWindow
        if (!window)
            return

        toolTip.close()
        window.close()
    }
    function hide()
    {
        close()
    }

    ToolTip {
        id: toolTip
        padding: 0
        anchors.centerIn: parent
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
