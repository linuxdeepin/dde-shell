// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control
    visible: false
    default property alias menuContent: menu.children
    property alias menuVisible: menu.visible
    property var menuWindow: Panel.menuWindow
    property int menuX: 0
    property int menuY: 0
    property bool readyBinding: false
    property bool openPending: false
    // WM_NAME, used for kwin.
    property string windowTitle: "dde-shell/panelmenu"
    width: menu.childrenRect.width
    height: menu.childrenRect.height

    Binding {
        when: readyBinding
        target: menuWindow; property: "requestedWidth"
        value: menu.width
    }
    Binding {
        when: readyBinding
        target: menuWindow; property: "requestedHeight"
        value: menu.height
    }
    Binding {
        when: readyBinding
        target: menuWindow; property: "xOffset"
        value: control.menuX
    }
    Binding {
        when: readyBinding
        target: menuWindow; property: "yOffset"
        value: control.menuY
    }

    function open()
    {
        if (menu.visible) {
            close()
            return
        }

        if (!menuWindow)
            return

        if (menuWindow.visible) {
            menuWindow.close()
            menuWindow.currentItem = null
            Qt.callLater(function () {
                if (!menu.visible) {
                    control.open()
                }
            })
            return
        }

        readyBinding = Qt.binding(function () {
            return menuWindow && menuWindow.currentItem === control
        })

        menuWindow.currentItem = control
        Qt.callLater(function () {
            if (!menuWindow || menuWindow.currentItem !== control)
                return
            openPending = true
            menuWindow.requestUpdateGeometry()
        })
    }

    function close()
    {
        openPending = false
        if (!menuWindow)
            return

        if (!readyBinding)
            return

        menuWindow.close()
        menuWindow.currentItem = null
        DS.grabKeyboard(menuWindow, false)
    }

    function finalizeOpen()
    {
        if (!menuWindow || !openPending || !readyBinding || menuWindow.currentItem !== control)
            return

        openPending = false
        menuWindow.title = windowTitle
        menuWindow.show()
        DS.grabMouse(menuWindow)
        DS.grabKeyboard(menuWindow)
    }

    Connections {
        target: menuWindow
        function onActiveChanged()
        {
            if (!menuWindow)
                return
            // TODO why activeChanged is not emit.
            if (menuWindow && !menuWindow.active) {
                control.close()
            }
        }

        function onUpdateGeometryFinished()
        {
            control.finalizeOpen()
        }
    }

    Item {
        id: menu
        visible: readyBinding
        width: control.width
        height: control.height
        parent: menuWindow ? menuWindow.contentItem : undefined
    }
    Component.onDestruction: {
        if (menu.visible)
            control.close()
    }
}
