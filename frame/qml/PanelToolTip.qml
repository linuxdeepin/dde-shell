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
    property bool readyBinding: false
    width: toolTip.width
    height: toolTip.height
    onToolTipVisibleChanged: {
        if (toolTipVisible) {
            open()
        } else {
            close()
        }
    }

    Binding {
        when: readyBinding
        target: toolTipWindow; property: "width"
        value: toolTip.width
    }
    Binding {
        when: readyBinding
        target: toolTipWindow; property: "height"
        value: toolTip.height
    }
    Binding {
        when: readyBinding
        target: toolTipWindow; property: "xOffset"
        value: control.toolTipX
    }
    Binding {
        when: readyBinding
        target: toolTipWindow; property: "yOffset"
        value: control.toolTipY
    }

    function open()
    {
        if (!toolTipWindow)
            return

        readyBinding = true
        Qt.callLater(function () {
            toolTip.open()
            toolTipWindow.show()
        })
    }
    
    function close()
    {
        if (!toolTipWindow)
            return

        readyBinding = false
        toolTip.close()
        toolTipWindow.close()
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
            if (!toolTipWindow)
                return
            toolTipWindow.visibleChanged.connect(function() {
                if (toolTipWindow && !toolTipWindow.visible)
                    toolTip.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
}
