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

        var rect = Qt.rect(control.x, control.y, popup.width, popup.height)
        window.setGeometry(rect)
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
        padding: 0
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
        // TODO dtk's blur causes blurred screen.
        background: null
    }
}
