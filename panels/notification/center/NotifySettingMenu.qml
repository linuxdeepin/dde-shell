// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

Menu {
    id: root

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
    modal: true

    Timer {
        id: closePolicyTimer
        interval: 1000
        onTriggered: {
            if (root.visible) {
                root.closePolicy = Popup.CloseOnPressOutside | Popup.CloseOnEscape
            }
        }
    }

    function toggle(isTouch)
    {
        if (!visible) {
            console.log("Open menu")
            if (isTouch) {
                closePolicy = Popup.CloseOnEscape
            }
            open()
            if (isTouch) {
                closePolicyTimer.restart()
            }
        } else {
            console.log("Close menu")
            close()
        }
    }

    onClosed: {
        closePolicyTimer.stop()
        closePolicy = Popup.CloseOnPressOutside | Popup.CloseOnEscape
    }
}
