// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.deepin.ds 1.0

Item {
    id: control
    visible: false
    default property alias popupContent: popup.children
    property alias popupVisible: popup.visible
    property var popupWindow: Panel.popupWindow
    property int popupX: 0
    property int popupY: 0
    property bool readyBinding: false
    property bool grabInactivePending: false
    property int grabInactiveTimeout: 200
    // WM_NAME, used for kwin.
    property string windowTitle: "dde-shell/panelpopup"
    width: popup.childrenRect.width
    height: popup.childrenRect.height

    Binding {
        when: readyBinding
        target: popupWindow; property: "width"
        value: popup.width
    }
    Binding {
        when: readyBinding
        target: popupWindow; property: "height"
        value: popup.height
    }
    Binding {
        when: readyBinding
        delayed: true
        target: popupWindow; property: "xOffset"
        value: control.popupX
    }
    Binding {
        when: readyBinding
        delayed: true
        target: popupWindow; property: "yOffset"
        value: control.popupY
    }

    function open()
    {
        if (popup.visible) {
            close()
            return
        }

        if (!popupWindow)
            return

        // The popup is being displayed. If you click on other plugin at this time,
        // the popup content of the previous plugin will be displayed first,
        // and the wrong popup size will cause the window size to change and flicker.
        if (popupWindow.visible) {
            popupWindow.close()
            popupWindow.currentItem = null
        }

        readyBinding = Qt.binding(function () {
            return popupWindow && popupWindow.currentItem === control
        })

        popupWindow.currentItem = control
        timer.start()
    }

    Timer {
        id: timer
        interval: 10
        onTriggered: {
            if (!popupWindow)
                return

            if (!readyBinding)
                return

            popupWindow.title = windowTitle
            popupWindow.show()
            popupWindow.requestActivate()
        }
    }
    Timer {
        id: grabInactiveTimer
        interval: control.grabInactiveTimeout
        repeat: false
        onTriggered: {
            control.grabInactivePending = false
            if (!popupWindow || !readyBinding || popupWindow.currentItem !== control || !popup.visible) {
                return
            }
            if (!popupWindow.active) {
                control.close()
            }
        }
    }
    function close()
    {
        grabInactivePending = false
        grabInactiveTimer.stop()
        if (!popupWindow)
            return

        // avoid to closing window by other PanelPopup.
        if (!readyBinding)
            return

        popupWindow.close()
        popupWindow.currentItem = null
    }

    Connections {
        target: popupWindow
        function onActiveChanged()
        {
            if (!popupWindow)
                return
            if (popupWindow.currentItem !== control || !popup.visible) {
                control.grabInactivePending = false
                grabInactiveTimer.stop()
                return
            }
            if (popupWindow.active) {
                control.grabInactivePending = false
                grabInactiveTimer.stop()
                return
            }
            if (control.grabInactivePending || popupWindow.x11GrabFocusTransition) {
                return
            }
            // TODO why activeChanged is not emit.
            if (!popupWindow.active) {
                control.close()
            }
        }

        function onX11FocusOutByGrab()
        {
            if (!popupWindow || !readyBinding || !popup.visible || popupWindow.currentItem !== control) {
                return
            }
            control.grabInactivePending = true
            grabInactiveTimer.start()
        }

        function onX11FocusInByUngrab()
        {
            if (!popupWindow || popupWindow.currentItem !== control || !control.grabInactivePending) {
                return
            }
            control.grabInactivePending = false
            grabInactiveTimer.stop()

            Qt.callLater(function() {
                if (!popupWindow
                        || !readyBinding
                        || popupWindow.currentItem !== control
                        || !popup.visible
                        || control.grabInactivePending) {
                    return
                }
                if (!popupWindow.active) {
                    control.close()
                }
            })
        }
    }

    Item {
        id: popup
        visible: readyBinding
        width: control.width
        height: control.height
        parent: popupWindow ? popupWindow.contentItem : undefined
    }
    Component.onDestruction: {
        if (popup.visible)
            control.close()
    }
}
