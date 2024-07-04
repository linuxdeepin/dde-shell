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
    property bool isHorizontal: false
    property bool collapsed: false

    role: "delegateType"
    LQM.DelegateChoice {
        roleValue: "dummy"
        TrayItemPositioner {
            visualSize: dummyDelegate.visualSize
            contentItem: DummyDelegate {
                id: dummyDelegate
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "legacy-tray-plugin"
        TrayItemPositioner {
            visualSize: traySurfaceDelegate.visualSize
            contentItem: ActionLegacyTrayPluginDelegate {
                id: traySurfaceDelegate
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-show-stash"
        TrayItemPositioner {
            contentItem: ActionShowStashDelegate {}
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-collapse"
        TrayItemPositioner {
            contentItem: ActionToggleCollapseDelegate {
                isHorizontal: root.isHorizontal
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-quick-settings"
        TrayItemPositioner {
            visualSize: Qt.size(quickSettingsDelegate.width, quickSettingsDelegate.height)
            ActionToggleQuickSettingsDelegate {
                id: quickSettingsDelegate
                isHorizontal: root.isHorizontal
            }
        }
    }
}
