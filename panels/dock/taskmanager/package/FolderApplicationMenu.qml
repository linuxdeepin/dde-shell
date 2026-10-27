// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Qt.labs.platform 1.1 as LP
import org.deepin.ds.dock 1.0

// Application menu inside the dock folder popup. Use DesktopIntegration's
// existing operations without changing the launcher's own menu implementation.
Item {
    id: root
    property var backend: null
    property var entry: ({id: "", name: ""})
    property bool busy: false
    signal launchRequested(string id)
    property bool onDesktop: false
    property bool docked: false
    property bool autoStart: false
    property bool canUninstall: false

    function openFor(item, provider) {
        entry = item
        backend = provider
        onDesktop = backend ? backend.isOnDesktop(item.id) : false
        docked = backend ? backend.isDockedApp(item.id) : false
        autoStart = backend ? backend.isAutoStart(item.id) : false
        canUninstall = backend && !backend.appIsCompulsoryForDesktop(item.id)
        busy = true
        MenuHelper.openMenu(menu)
    }

    LP.Menu {
        id: menu
        onAboutToHide: Qt.callLater(function() { if (!confirmation.visible) root.busy = false })
        LP.MenuItem {
            text: qsTr("Open")
            onTriggered: root.launchRequested(root.entry.id)
        }
        LP.MenuItem {
            enabled: !!root.backend
            text: root.onDesktop ? qsTr("Remove from desktop") : qsTr("Send to desktop")
            onTriggered: root.onDesktop ? root.backend.removeFromDesktop(root.entry.id) : root.backend.sendToDesktop(root.entry.id)
        }
        LP.MenuItem {
            enabled: !!root.backend
            text: root.docked ? qsTr("Remove from dock") : qsTr("Send to dock")
            onTriggered: root.docked ? root.backend.removeFromDock(root.entry.id) : root.backend.sendToDock(root.entry.id)
        }
        LP.MenuItem {
            enabled: !!root.backend
            text: root.autoStart ? qsTr("Remove from startup") : qsTr("Add to startup")
            onTriggered: root.backend.setAutoStart(root.entry.id, !root.autoStart)
        }
        LP.MenuSeparator {}
        LP.MenuItem {
            enabled: root.canUninstall
            text: qsTr("Uninstall")
            onTriggered: {
                if (root.backend.shouldSkipConfirmUninstallDialog(root.entry.id)) {
                    root.backend.uninstallApp(root.entry.id)
                } else {
                    confirmation.open()
                }
            }
        }
    }
    LP.MessageDialog {
        id: confirmation
        title: qsTr("Uninstall")
        text: qsTr("Uninstall %1?").arg(root.entry.name)
        buttons: LP.MessageDialog.Yes | LP.MessageDialog.No
        onYesClicked: { root.backend.uninstallApp(root.entry.id); root.busy = false }
        onNoClicked: root.busy = false
        onRejected: root.busy = false
    }
}
