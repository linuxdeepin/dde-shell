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

        window.x = control.x
        window.y = control.y
        window.width = popup.width
        window.height = popup.height
        window.show()
        popup.open()
    }
    function close()
    {
        var window = Panel.popupWindow
        if (!window)
            return

        window.close()
        popup.close()
    }

    Popup {
        id: popup
        width: control.width
        height: control.height
        parent: Panel.popupWindow ? Panel.popupWindow.contentItem : undefined
        onParentChanged: function() {
            var window = Panel.popupWindow
            if (!window)
                return
            window.visibleChanged.connect(function() {
                if (!Panel.popupWindow.visible)
                    popup.close()
            })
        }
    }
}
