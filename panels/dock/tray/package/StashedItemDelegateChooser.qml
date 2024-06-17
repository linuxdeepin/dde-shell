// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1 as LP
import Qt.labs.qmlmodels 1.2 as LQM // qml6-module-qt-labs-qmlmodels
import org.deepin.ds.dock.tray 1.0 as DDT

LQM.DelegateChooser {
    id: root

    required property int columnCount
    required property int rowCount

    role: "delegateType"
    LQM.DelegateChoice {
        roleValue: "dummy"
        StashedItemPositioner {
            contentItem: DummyDelegate {}
        }
    }
    LQM.DelegateChoice {
        roleValue: "legacy-tray-plugin"
        StashedItemPositioner {
            contentItem: ActionLegacyTrayPluginDelegate {
                inputEventsEnabled: false // temporary
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-show-stash"
        StashedItemPositioner {
            contentItem: ActionShowStashDelegate {}
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-collapse"
        StashedItemPositioner {
            contentItem: ActionToggleCollapseDelegate {
                isHorizontal: root.isHorizontal
            }
        }
    }
}
