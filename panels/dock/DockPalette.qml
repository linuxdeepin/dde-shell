// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick 2.15

import org.deepin.ds.dock 1.0
import org.deepin.dtk as D

Item {
    property D.Palette toolButtonColor: D.Palette {
        normal {
            common: Qt.rgba(1, 1, 1, 0.4)
        }
        normalDark{
            common: Qt.rgba(1, 1, 1, 0.1)
        }
    }

    property D.Palette toolButtonBorderColor: D.Palette {
        normal {
            common: Qt.rgba(0, 0, 0, 0.1)
        }
        normalDark{
            common: Qt.rgba(1, 1, 1, 0.2)
        }
    }

    property D.Palette workspaceRectangleColor: D.Palette {
        normal {
            common: Qt.rgba(0, 0, 0, 0.1)
        }

        normalDark {
            common: Qt.rgba(1, 1, 1, 0.1)
        }

        hovered {
            common: Qt.rgba(0, 0, 0, 0.2)
        }

        hoveredDark {
            common: Qt.rgba(0, 0, 0, 0.3)
        }
    }

    property D.Palette workspaceSelectedBorderColor: D.Palette {
        normal {
            common: Qt.rgba(0, 0, 0, 1)
        }
        normalDark{
            common: Qt.rgba(1, 1, 1, 1)
        }
    }

    property D.Palette workspaceUnselectedBorderColor: D.Palette {
        normal {
            common: Qt.rgba(0, 0, 0, 0.5)
        }
        normalDark {
            common: Qt.rgba(1, 1, 1, 0.5)
        }

        hovered {
            common: Qt.rgba(0, 0, 0, 0.6)
        }

        hoveredDark{
            common: Qt.rgba(1, 1, 1, 0.6)
        }
    }

    property D.Palette taskmanagerStatusIndicatorColor: D.Palette {

    }

    property D.Palette taskmanagerStatusIndicatorBorderColor: D.Palette {

    }

    property D.Palette taskmanagerWindowIndicatorColor: D.Palette {

    }

    property D.Palette taskmanagerWindowIndicatorBoderColor: D.Palette {

    }

}
