// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Qt.labs.platform 1.1 as LP
import org.deepin.ds.dock 1.0

// Source-neutral menu for the PINNED FOLDER. The registry supplies the two
// framework actions followed by optional actions from its source plugin.
LP.Menu {
    id: root
    property var registry
    property string folderType
    property string folderId
    property var entries: []
    signal openRequested()
    signal undockRequested()

    function openFor(type, id, name) {
        folderType = type
        folderId = id
        entries = registry.folderMenuItems(type, id, name)
        MenuHelper.openMenu(root)
    }

    Instantiator {
        model: root.entries
        delegate: LP.MenuItem {
            text: modelData.name
            enabled: modelData.enabled !== false
            checkable: modelData.checkable === true
            checked: modelData.checked === true
            onTriggered: {
                if (modelData.id === "open-folder") root.openRequested()
                else if (modelData.id === "undock-folder") root.undockRequested()
                else root.registry.triggerFolderMenuAction(root.folderType, root.folderId, modelData.id)
            }
        }
        onObjectAdded: (index, object) => root.insertItem(index, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }
}
