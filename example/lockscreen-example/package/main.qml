// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import org.deepin.ds 1.0

AppletItem {
    implicitWidth: 120
    implicitHeight: 100

    function lockscreenApplet()
    {
        var lockscreen = DS.applet("org.deepin.ds.dde-shutdown")
        if (lockscreen) {
            return lockscreen
        } else {
            console.warn("shutdown applet not found")
        }
    }

    Column {
        Button {
            text: "Lock"
            onClicked: {
                let lockscreen = lockscreenApplet()
                if (lockscreen) {
                    lockscreen.requestShutdown("Lock")
                }
            }
        }
        Button {
            text: "Shutdown"
            onClicked: {
                let lockscreen = lockscreenApplet()
                if (lockscreen) {
                    lockscreen.requestShutdown("Shutdown")
                }
            }
        }
        Button {
            text: "SwitchUser"
            onClicked: {
                let lockscreen = lockscreenApplet()
                if (lockscreen) {
                    lockscreen.requestShutdown("SwitchUser")
                }
            }
        }
    }
}
