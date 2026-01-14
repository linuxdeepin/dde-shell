// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton
import QtQuick
import Qt.labs.platform

Item {
    property Menu activeMenu: null
    property var aboutToHideConnections: ({})  // Store connections by menu object

    function openMenu(menu: Menu) {
        if (activeMenu) {
            activeMenu.close()
        }

        // Disconnect previous connection for this menu if exists
        if (aboutToHideConnections[menu]) {
            try {
                menu.aboutToHide.disconnect(aboutToHideConnections[menu])
            } catch (e) {
                // Silently ignore disconnect errors
            }
        }
        menu.open()

        // Create and store the handler for this specific menu
        let handler = function() {
            activeMenu = null
        }
        aboutToHideConnections[menu] = handler
        menu.aboutToHide.connect(handler)

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
