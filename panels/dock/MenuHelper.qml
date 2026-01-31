// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton
import QtQuick
import Qt.labs.platform

Item {
    property Menu activeMenu: null
    
    signal menuClosed()
    Connections {
        target: activeMenu
        function onAboutToHide() {
            activeMenu = null
            menuClosed()
        }
    }
    function openMenu(menu: Menu) {
        if (activeMenu) {
            activeMenu.close()
        }
        menu.open()
        activeMenu = menu
    }
    function closeMenu(menu: Menu) {
        menu.close()
    }
    function closeCurrent() {
        if (activeMenu) {
           closeMenu(activeMenu)
        }
    }
}
