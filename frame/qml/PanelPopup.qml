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
    function open()
    {
        var window = Panel.popupWindow
        if (!window)
            return

        window.width = Qt.binding(function() {
            return popup.width + popup.leftPadding + popup.rightPadding
        })
        window.height = Qt.binding(function() {
            return popup.height + popup.topPadding + popup.bottomPadding
        })

        window.xOffset = Qt.binding(function(){
            return control.x
        })
        window.yOffset = Qt.binding(function(){
            return control.y
        })
        window.show()
        popup.open()
        window.requestActivate()
    }
    function close()
    {
        var window = Panel.popupWindow
        if (!window)
            return

        window.close()
        popup.close()
    }

    Connections {
        target: Panel.popupWindow
        function onActiveChanged()
        {
            var window = Panel.popupWindow
            if (!window)
                return
            // TODO why activeChanged is not emit.
            if (Panel.popupWindow && !Panel.popupWindow.active) {
                control.close()
            }
        }
    }

    Popup {
        id: popup
        padding: 0
        width: control.width
        height: control.height
        parent: Panel.popupWindow ? Panel.popupWindow.contentItem : undefined
        onParentChanged: function() {
            var window = Panel.popupWindow
            if (!window)
                return
            window.visibleChanged.connect(function() {
                if (Panel.popupWindow && !Panel.popupWindow.visible)
                    control.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
    Component.onDestruction: control.close()
}
