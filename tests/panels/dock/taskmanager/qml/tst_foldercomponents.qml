// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../../../../panels/dock/taskmanager/package" as TaskItems

TestCase {
    name: "FolderComponents"
    when: windowShown
    visible: true
    width: 100; height: 100
    TaskItems.FolderIcon {
        id: icon
        width: 64; height: 64
    }
    function test_preview_data() {
        return [{tag: "empty", count: 0}, {tag: "one", count: 1},
                {tag: "four", count: 4}, {tag: "overflow", count: 8}]
    }
    function test_preview(data) {
        let entries = []
        for (let i = 0; i < data.count; ++i)
            entries.push({id: "app" + i, name: "App " + i, iconName: "folder"})
        icon.contents = entries
        for (let slot = 0; slot < 4; ++slot) {
            let preview = findChild(icon, "folderPreview" + slot)
            verify(preview !== null)
            compare(preview.visible, slot < data.count)
            if (slot < data.count) compare(preview.name, "folder")
        }
        // Empty folders must still read as a folder instead of a blank square.
        let placeholder = findChild(icon, "folderPlaceholder")
        verify(placeholder !== null)
        compare(placeholder.visible, data.count === 0)
        compare(placeholder.name, "folder")
        // ...and they must not paint the translucent preview plate behind it.
        compare(icon.color.a, data.count === 0 ? 0 : 0.2)
    }
    Component {
        id: folderMenuComponent
        TaskItems.FolderContextMenu {}
    }
    QtObject {
        id: menuRegistry
        property var dispatched: []
        function triggerFolderMenuAction(type, folderId, actionId) {
            dispatched = dispatched.concat([{type: type, folderId: folderId, actionId: actionId}])
        }
    }
    SignalSpy { id: openSpy; signalName: "openRequested" }
    SignalSpy { id: undockSpy; signalName: "undockRequested" }

    function test_folderMenuDispatch() {
        const menu = createTemporaryObject(folderMenuComponent, this, {
            registry: menuRegistry,
            folderType: "launcher",
            folderId: "internal/folders/12",
            entries: [{id: "open-folder", name: "My folder"},
                      {id: "undock-folder", name: "Undock"}]
        })
        verify(menu !== null)
        openSpy.target = menu
        undockSpy.target = menu
        openSpy.clear()
        undockSpy.clear()
        menuRegistry.dispatched = []
        tryCompare(menu.items, "length", 2)
        compare(menu.items[0].text, "My folder")
        compare(menu.items[0].enabled, true)
        menu.items[0].triggered()
        compare(openSpy.count, 1)
        compare(undockSpy.count, 0)
        menu.items[1].triggered()
        compare(openSpy.count, 1)
        compare(undockSpy.count, 1)
        compare(menuRegistry.dispatched.length, 0)

        menu.entries = menu.entries.concat([
            {id: "details", name: "Details", checkable: true, checked: true},
            {id: "disabled", name: "Unavailable", enabled: false}
        ])
        tryCompare(menu.items, "length", 4)
        compare(menu.items[2].checkable, true)
        compare(menu.items[2].checked, true)
        compare(menu.items[3].enabled, false)
        menu.items[2].triggered()
        compare(menuRegistry.dispatched.length, 1)
        compare(menuRegistry.dispatched[0].type, "launcher")
        compare(menuRegistry.dispatched[0].folderId, "internal/folders/12")
        compare(menuRegistry.dispatched[0].actionId, "details")
        compare(openSpy.count, 1)
        compare(undockSpy.count, 1)
        openSpy.target = null
        undockSpy.target = null
    }
    function test_popupAndMenuCompile() {
        for (const name of ["FolderPopup.qml", "FolderContextMenu.qml", "FolderApplicationMenu.qml"]) {
            const component = Qt.createComponent(Qt.resolvedUrl("../../../../../panels/dock/taskmanager/package/" + name))
            compare(component.status, Component.Ready, component.errorString())
            component.destroy()
        }
    }
}
