// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LGPL-3.0-or-later
import QtQuick
import QtTest
import "../../../frame/qml" as Shell

TestCase {
    name: "PanelPopupLifecycle"
    when: windowShown
    // Control focus explicitly: these tests exercise popup policy, not the WM.
    QtObject {
        id: sharedWindow
        property var currentItem: null
        property Item contentItem: Item {}
        property real requestedWidth: 120
        property real requestedHeight: 80
        property real xOffset: 0
        property real yOffset: 0
        property bool visible: false
        property bool active: false
        property real opacity: 1
        property string title: ""
        property bool x11GrabFocusTransition: false
        signal updateGeometryFinished()
        signal frameSwapped()
        signal x11FocusOutByGrab()
        signal x11FocusInByUngrab()
        function requestUpdateGeometry() { updateGeometryFinished() }
        function show() { visible = true }
        function close() { visible = false; active = false }
        function requestActivate() { active = true }
        function update() { Qt.callLater(function() { sharedWindow.frameSwapped() }) }
    }
    Shell.PanelPopup {
        id: first
        popupWindow: sharedWindow
        width: 120; height: 80
        grabInactiveTimeout: 20
        Rectangle { anchors.fill: parent; color: "red" }
    }
    Shell.PanelPopup {
        id: second
        popupWindow: sharedWindow
        width: 140; height: 90
    }
    function cleanup() {
        first.close()
        second.close()
        first.closeOnInactive = true
        second.closeOnInactive = true
        first.revealAfterRender = false
        wait(0)
    }
    function test_openCloseAndToggle() {
        first.open()
        tryCompare(sharedWindow, "visible", true)
        compare(sharedWindow.opacity, 1)
        compare(sharedWindow.currentItem, first)
        first.open() // A second click closes immediately, without an animation.
        compare(sharedWindow.currentItem, null)
        compare(sharedWindow.visible, false)
        compare(sharedWindow.opacity, 1)
    }
    function test_switchSharedWindow() {
        first.open()
        tryCompare(sharedWindow, "visible", true)
        second.open()
        tryCompare(sharedWindow, "currentItem", second)
        tryCompare(sharedWindow, "visible", true)
        compare(sharedWindow.opacity, 1)
        // A stale close from the previous folder must not close the new owner.
        first.close()
        compare(sharedWindow.currentItem, second)
        compare(sharedWindow.visible, true)
    }
    function test_revealAfterRender() {
        first.revealAfterRender = true
        first.open()
        tryCompare(sharedWindow, "visible", true)
        tryCompare(first, "revealPending", false)
        compare(sharedWindow.opacity, 1)
        compare(sharedWindow.active, true)
        first.close()
        compare(sharedWindow.visible, false)
        compare(sharedWindow.currentItem, null)
    }
    function test_focusLoss_data() {
        let cases = []
        for (const source of ["activeChanged", "grabTimeout", "ungrab"]) {
            for (const close of [true, false])
                cases.push({tag: source + "-" + close, source: source, close: close})
        }
        return cases
    }
    function test_focusLoss(data) {
        first.closeOnInactive = data.close
        first.open()
        tryCompare(sharedWindow, "visible", true)
        compare(sharedWindow.active, true)
        if (data.source !== "activeChanged") sharedWindow.x11FocusOutByGrab()
        sharedWindow.active = false
        if (data.source === "ungrab") sharedWindow.x11FocusInByUngrab()
        if (data.close) {
            tryCompare(sharedWindow, "currentItem", null)
            compare(sharedWindow.visible, false)
        } else {
            if (data.source === "grabTimeout") tryCompare(first, "grabInactivePending", false)
            wait(30) // Process deferred focus callbacks before checking visibility.
            compare(sharedWindow.currentItem, first)
            compare(sharedWindow.visible, true)
        }
    }
    function test_interactionEndsWhileInactive() {
        first.closeOnInactive = false
        first.open()
        tryCompare(sharedWindow, "visible", true)
        sharedWindow.active = false
        wait(30)
        compare(sharedWindow.visible, true)
        first.closeOnInactive = true
        tryCompare(sharedWindow, "currentItem", null)
        compare(sharedWindow.visible, false)
    }
    function test_interactionEndsAfterFocusReturns() {
        first.closeOnInactive = false
        first.open()
        tryCompare(sharedWindow, "visible", true)
        sharedWindow.active = false
        first.closeOnInactive = true
        sharedWindow.active = true
        wait(30)
        compare(sharedWindow.currentItem, first)
        compare(sharedWindow.visible, true)
    }
}
