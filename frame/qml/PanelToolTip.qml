// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control

    property alias toolTipContent: toolTip.contentChildren
    default property alias toolTipContentItem: toolTip.contentItem
    property alias text: toolTip.text
    property alias toolTipVisible: toolTip.visible
    property var toolTipWindow: Panel.toolTipWindow
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
        var window = toolTipWindow
        if (!window)
            return
        window.width = Qt.binding(function() {
            return toolTip.width + toolTip.leftPadding + toolTip.rightPadding
        })
        window.height = Qt.binding(function() {
            return toolTip.height + toolTip.topPadding + toolTip.bottomPadding
        })

        window.xOffset = Qt.binding(function() {
            return control.toolTipX - toolTip.width / 2
        })
        window.yOffset = Qt.binding(function() {
            return control.toolTipY - toolTip.height - control.margins
        })
        window.show()
        toolTip.open()
    }
    
    function close()
    {
        var window = toolTipWindow
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
        // TODO it's a bug for qt, ToolTip's text color can't change with window's palette changed.
        palette.toolTipText: toolTipWindow ? toolTipWindow.palette.toolTipText : undefined
        anchors.centerIn: parent
        topPadding: 0
        bottomPadding: 0
        parent: toolTipWindow ? toolTipWindow.contentItem : undefined
        onParentChanged: function() {
            var window = toolTipWindow
            if (!window)
                return
            window.visibleChanged.connect(function() {
                if (toolTipWindow && !toolTipWindow.visible)
                    toolTip.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
}
