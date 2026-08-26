// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1 as LP
import Qt.labs.qmlmodels 1.2 as LQM // for Qt < 6.9
import QtQml.Models as LQM // for Qt >= 6.9
import org.deepin.ds.dock.tray 1.0 as DDT

LQM.DelegateChooser {
    id: root
    property bool isHorizontal: false
    property bool collapsed: false
    required property int itemPadding
    required property var surfaceAcceptor
    property bool disableInputEvents

    role: "delegateType"
    LQM.DelegateChoice {
        roleValue: "dummy"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Accessible.id: "TrayItemPositioner"
            visualSize: dummyDelegate.visualSize
            contentItem: DummyDelegate {
                Accessible.role: Accessible.Button
                Accessible.id: "DummyDelegate"
                id: dummyDelegate
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "legacy-tray-plugin"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Accessible.id: "TraySurfacePositioner"
            id: traySurfacePositioner
            visualSize: traySurfaceDelegate.visualSize
            contentItem: ActionLegacyTrayPluginDelegate {
                Accessible.role: Accessible.Button
                Accessible.id: "TraySurfaceDelegate"
                id: traySurfaceDelegate
                objectName: "tray"
                inputEventsEnabled: !disableInputEvents && (model.sectionType !== "collapsable" || !DDT.TraySortOrderModel.isCollapsing)
                itemVisible: traySurfacePositioner.itemVisible
                dragable: model.sectionType !== "fixed"
                isActive: surfacePopup.isOpened

                // trayItem's popup
                DDT.TrayItemSurfacePopup {
                    Accessible.role: Accessible.Dialog
                    Accessible.id: "SurfacePopup"
                    id: surfacePopup
                    surfaceAcceptor: function (surfaceId) {
                        if (root.surfaceAcceptor && !root.surfaceAcceptor(surfaceId))
                            return false

                        return surfaceId === model.surfaceId
                    }
                }
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-show-stash"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Accessible.id: "TrayItemPositioner"
            contentItem: ActionShowStashDelegate {
                Accessible.role: Accessible.Button
                Accessible.id: "ActionShowStashDelegate"
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-collapse"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Accessible.id: "TrayItemPositioner"
            contentItem: ActionToggleCollapseDelegate {
                Accessible.role: Accessible.Button
                Accessible.id: "ActionToggleCollapseDelegate"
                isHorizontal: root.isHorizontal
                inputEventsEnabled: !disableInputEvents
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-quick-settings"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Accessible.id: "TrayItemPositioner"
            visualSize: Qt.size(quickSettingsDelegate.width, quickSettingsDelegate.height)
            ActionToggleQuickSettingsDelegate {
                Accessible.role: Accessible.Button
                Accessible.id: "QuickSettingsDelegate"
                id: quickSettingsDelegate
                isHorizontal: root.isHorizontal
            }
        }
    }
}
