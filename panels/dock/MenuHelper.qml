// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton
import QtQuick
import org.deepin.ds.dock 1.0

Item {
    id: root

    property var activeMenu: null
    
    signal menuClosed()
    Connections {
        target: root.activeMenu
        function onAboutToHide() {
            root.activeMenu = null
            root.menuClosed()
        }
    }
    function openMenu(menu, parentItem, point) {
        if (!menu) {
            return
        }

        if (activeMenu) {
            activeMenu.close()
        }

        if (parentItem !== undefined && point !== undefined) {
            menu.popup(parentItem, point.x, point.y)
        } else {
            menu.open()
        }
        activeMenu = menu
    }
    function calculateMenuPosition(point, menu, position) {
        let menuWidth = Math.max(menu.implicitWidth, menu.width, 1)
        let menuHeight = Math.max(menu.implicitHeight, menu.height, 1)
        switch (position) {
        case Dock.Right:
            return Qt.point(point.x - menuWidth, point.y)
        case Dock.Bottom:
            return Qt.point(point.x, point.y - menuHeight)
        case Dock.Left:
        case Dock.Top:
        default:
            return point
        }
    }
    function closeMenu(menu) {
        if (menu) {
            menu.close()
        }
    }
    function closeCurrent() {
        if (activeMenu) {
           closeMenu(activeMenu)
        }
    }
}
