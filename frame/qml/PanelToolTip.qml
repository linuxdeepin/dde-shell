// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.dtk.style as DStyle

Item {
    id: control

    default property alias toolTipContentItem: toolTip.contentItem
    property string text
    property alias toolTipVisible: toolTip.visible
    property var toolTipWindow: Panel.toolTipWindow
    property int toolTipX: 0
    property int toolTipY: 0
    property bool readyBinding: false
    property bool openPending: false
    // WM_NAME, used for kwin.
    property string windowTitle: "dde-shell/paneltooltip"
    width: toolTip.width
    height: toolTip.height

    Binding {
        when: readyBinding
        target: toolTipWindow; property: "requestedWidth"
        value: toolTip.width + toolTip.leftPadding + toolTip.rightPadding
    }
    Binding {
        when: readyBinding
        target: toolTipWindow; property: "requestedHeight"
        value: toolTip.height
    }
    Binding {
        when: readyBinding
        target: toolTipWindow; property: "xOffset"
        value: control.toolTipX - (toolTip.leftPadding + toolTip.rightPadding) / 2
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

        if (toolTipWindow.visible) {
            toolTipWindow.close()
            toolTipWindow.currentItem = null
            Qt.callLater(function () {
                if (!toolTip.visible) {
                    control.open()
                }
            })
            return
        }

        readyBinding = Qt.binding(function () {
            return toolTipWindow && toolTipWindow.currentItem === control
        })

        toolTipWindow.currentItem = control
        openPending = true
        Qt.callLater(function () {
            if (!toolTipWindow || !openPending || !readyBinding || toolTipWindow.currentItem !== control)
                return

            toolTipWindow.requestUpdateGeometry()
        })
    }

    function close()
    {
        openPending = false
        if (!toolTipWindow)
            return

        if (!readyBinding)
            return

        toolTipWindow.close()
        toolTipWindow.currentItem = null
    }

    function hide()
    {
        close()
    }

    function finalizeOpen()
    {
        if (!toolTipWindow || !openPending || !readyBinding || toolTipWindow.currentItem !== control)
            return

        openPending = false
        toolTipWindow.title = windowTitle
        toolTipWindow.show()
    }

    Connections {
        target: toolTipWindow
        function onUpdateGeometryFinished()
        {
            control.finalizeOpen()
        }
    }

    Control {
        id: toolTip
        visible: readyBinding
        anchors.centerIn: parent
        parent: toolTipWindow ? toolTipWindow.contentItem : undefined
        font {
            family: DTK.fontManager.t8.family
            pixelSize: DTK.fontManager.t8.pixelSize
            capitalization: Font.Capitalize
        }
        contentItem: Text {
            topPadding: 4
            bottomPadding: 4
            leftPadding: 8
            rightPadding: 8

            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font: toolTip.font
            wrapMode: Text.WordWrap
            text: control.text
            color: toolTip.palette.brightText
        }
    }
}
