// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.dtk 1.0
import org.deepin.ds.notification

FloatingPanel {
    id: root
    radius: 12
    backgroundColor: Palette {
        normal {
            common: ("transparent")
            crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.7)
        }
        normalDark {
            crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 0.7)
        }
    }
    insideBorderColor: Palette {
        normal {
            common: ("transparent")
            crystal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.2)
        }
        normalDark {
            crystal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.1)
        }
    }
    outsideBorderColor: Palette {
        normal {
            common: ("transparent")
            crystal: Qt.rgba(0, 0, 0, 0.1)
        }
        normalDark {
            crystal: Qt.rgba(0, 0, 0, 0.6)
        }
    }
    dropShadowColor: Palette {
        normal {
            common: ("transparent")
            crystal: Qt.rgba(0, 0, 0, 0.2)
        }
        normalDark {
            crystal: Qt.rgba(0, 0, 0, 0.4)
        }
    }
}
