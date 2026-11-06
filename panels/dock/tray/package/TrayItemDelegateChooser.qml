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
            Accessible.id: "TrayItemDelegateChooser_Pane"
            Component.onCompleted: { Accessible.id = "TrayItemPositioner" }
            visualSize: dummyDelegate.visualSize
            contentItem: DummyDelegate {
                Accessible.id: "TrayItemDelegateChooser_Pane_2"
                Accessible.role: Accessible.Button
                Component.onCompleted: { Accessible.id = "DummyDelegate" }
                id: dummyDelegate
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "legacy-tray-plugin"
        TrayItemPositioner {
            Accessible.id: "TrayItemDelegateChooser_Pane_5"
            Component.onCompleted: { Accessible.id = "TraySurfacePositioner" }
            id: traySurfacePositioner
            visualSize: traySurfaceDelegate.visualSize
            contentItem: ActionLegacyTrayPluginDelegate {
                Accessible.id: "TrayItemDelegateChooser_Pane_4"
                Accessible.role: Accessible.Button
                Component.onCompleted: { Accessible.id = "TraySurfaceDelegate" }
                id: traySurfaceDelegate
                objectName: "tray"
                inputEventsEnabled: !disableInputEvents && (model.sectionType !== "collapsable" || !DDT.TraySortOrderModel.isCollapsing)
                itemVisible: traySurfacePositioner.itemVisible
                dragable: model.sectionType !== "fixed"
                isActive: surfacePopup.isOpened

                // trayItem's popup
                DDT.TrayItemSurfacePopup {
                    Accessible.id: "TrayItemDelegateChooser_Dialog"
                    Accessible.role: Accessible.Dialog
                    Component.onCompleted: { Accessible.id = "SurfacePopup" }
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
        Accessible.id: "TrayItemDelegateChooser_Pane_13"
        roleValue: "action-show-stash"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Component.onCompleted: { Accessible.id = "TrayItemPositioner" }
            contentItem: ActionShowStashDelegate {
                Accessible.id: "TrayItemDelegateChooser_Pane_6"
                Accessible.role: Accessible.Button
                Component.onCompleted: { Accessible.id = "ActionShowStashDelegate" }
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-collapse"
        TrayItemPositioner {
            Accessible.role: Accessible.Pane
            Component.onCompleted: { Accessible.id = "TrayItemPositioner" }
            contentItem: ActionToggleCollapseDelegate {
                Accessible.id: "TrayItemDelegateChooser_Pane_8"
                Accessible.role: Accessible.Button
                Component.onCompleted: { Accessible.id = "ActionToggleCollapseDelegate" }
                isHorizontal: root.isHorizontal
                inputEventsEnabled: !disableInputEvents
            }
        }
    }
    LQM.DelegateChoice {
        roleValue: "action-toggle-quick-settings"
        TrayItemPositioner {
            Accessible.id: "TrayItemDelegateChooser_Pane_12"
            Accessible.role: Accessible.Pane
            Component.onCompleted: { Accessible.id = "TrayItemPositioner" }
            visualSize: Qt.size(quickSettingsDelegate.width, quickSettingsDelegate.height)
            ActionToggleQuickSettingsDelegate {
                Accessible.id: "TrayItemDelegateChooser_Pane_11"
                Accessible.role: Accessible.Button
                Component.onCompleted: { Accessible.id = "QuickSettingsDelegate" }
                id: quickSettingsDelegate
                isHorizontal: root.isHorizontal
            }
        }
    }
}


