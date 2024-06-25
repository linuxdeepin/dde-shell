// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control
    visible: false
    default property alias popupContent: popup.contentChildren
    property alias popupVisible: popup.visible
    property var popupWindow: Panel.popupWindow
    property int popupX: 0
    property int popupY: 0
    property bool readyBinding: false

    Binding {
        when: readyBinding
        target: popupWindow; property: "width"
        value: popup.width
    }
    Binding {
        when: readyBinding
        target: popupWindow; property: "height"
        value: popup.height
    }
    Binding {
        when: readyBinding
        target: popupWindow; property: "xOffset"
        value: control.popupX
    }
    Binding {
        when: readyBinding
        target: popupWindow; property: "yOffset"
        value: control.popupY
    }

    function open()
    {
        if (popup.visible) {
            close()
            return
        }

        if (!popupWindow)
            return

        readyBinding = true
        Qt.callLater(function () {
            popupWindow.show()
            popup.open()
            popupWindow.requestActivate()
        })

    }
    function close()
    {
        if (!popupWindow)
            return

        readyBinding = false
        popupWindow.close()
        popup.close()
    }

    Connections {
        target: popupWindow
        function onActiveChanged()
        {
            if (!popupWindow)
                return
            // TODO why activeChanged is not emit.
            if (popupWindow && !popupWindow.active) {
                control.close()
            }
        }
    }

    Popup {
        id: popup
        padding: 0
        width: control.width
        height: control.height
        parent: popupWindow ? popupWindow.contentItem : undefined
        onParentChanged: function() {
            if (!popupWindow)
                return
            popupWindow.visibleChanged.connect(function() {
                if (popupWindow && !popupWindow.visible)
                    control.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
    Component.onDestruction: {
        if (popup.visible)
            control.close()
    }
}
