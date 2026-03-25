// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.dtk 1.0
import Qt.labs.platform 1.1 as LP
import org.deepin.ds.notification

LP.Menu {
    id: root

    property var actions: []

    signal actionInvoked(var actionId)

    Instantiator {
        model: root.actions
        delegate: LP.MenuItem {
            text: modelData.text
            onTriggered: {
                console.log("Action triggered: " + modelData.text + " (ID: " + modelData.id + ")")
                root.actionInvoked(modelData.id)
            }
        }
        onObjectAdded: function(index, object) {
            root.insertItem(index, object)
        }
        onObjectRemoved: function(index, object) {
            root.removeItem(object)
        }
    }
}
