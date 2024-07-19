// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control
    visible: false
    default property alias menuContent: menu.contentChildren
    property alias menuVisible: menu.visible
    property var menuWindow: Panel.menuWindow
    property int menuX: 0
    property int menuY: 0
    property bool readyBinding: false

    Binding {
        when: readyBinding
        target: menuWindow; property: "width"
        value: menu.width
    }
    Binding {
        when: readyBinding
        target: menuWindow; property: "height"
        value: menu.height
    }
    Binding {
        when: readyBinding
        delayed: true
        target: menuWindow; property: "xOffset"
        value: control.menuX
    }
    Binding {
        when: readyBinding
        delayed: true
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

        readyBinding = Qt.binding(function () {
            return menuWindow && menuWindow.currentItem === control
        })

        menuWindow.currentItem = control
        Qt.callLater(function () {
            DS.grabMouse(menuWindow)
            menuWindow.show()
        })
    }

    function close()
    {
        if (!menuWindow)
            return

        menuWindow.currentItem = null
        menuWindow.close()
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
    }

    Popup {
        id: menu
        padding: 0
        visible: readyBinding
        width: control.width
        height: control.height
        parent: menuWindow ? menuWindow.contentItem : undefined
        onParentChanged: function() {
            if (!menuWindow)
                return
            menuWindow.visibleChanged.connect(function() {
                if (menuWindow && !menuWindow.visible)
                    control.close()
            })
        }
        // TODO dtk's blur causes blurred screen.
        background: null
    }
    Component.onDestruction: {
        if (menu.visible)
            control.close()
    }
}
