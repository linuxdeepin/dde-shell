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

    property int paddings: 0
    property int margins: 10
    property int popupX: 0
    property int popupY: 0

    function open()
    {
        if (popup.visible) {
            close()
            return
        }

        var window = popupWindow
        if (!window)
            return

        window.width = Qt.binding(function() {
            return popup.width + popup.leftPadding + popup.rightPadding
        })
        window.height = Qt.binding(function() {
            return popup.height + popup.topPadding + popup.bottomPadding
        })

        window.xOffset = Qt.binding(function() {
            return control.popupX - window.width / 2
        })
        window.yOffset = Qt.binding(function() {
            return control.popupY - window.height - control.margins
        })
        window.show()
        popup.open()
        window.requestActivate()
    }
    function close()
    {
        var window = popupWindow
        if (!window)
            return

        window.close()
        popup.close()
    }

    Connections {
        target: popupWindow
        function onActiveChanged()
        {
            var window = popupWindow
            if (!window)
                return
            // TODO why activeChanged is not emit.
            if (popupWindow && !popupWindow.active) {
                control.close()
            }
        }
    }

    Popup {
        id: popup
        padding: control.paddings
        width: control.width
        height: control.height
        parent: popupWindow ? popupWindow.contentItem : undefined
        onParentChanged: function() {
            var window = popupWindow
            if (!window)
                return
            window.visibleChanged.connect(function() {
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
