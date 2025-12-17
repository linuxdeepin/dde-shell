// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

FocusScope {
    id: root
    activeFocusOnTab: true

    required property NotifyModel notifyModel

    signal gotoFirstNotify()  // Signal to Tab to first notify item
    signal gotoLastNotify()   // Signal to Shift+Tab to last notify item

    // Forward focus to first button when FocusScope receives focus
    onActiveFocusChanged: {
        if (activeFocus && !collapseBtn.activeFocus && !moreBtn.activeFocus && !clearAllBtn.activeFocus) {
            if (collapseBtn.visible) {
                collapseBtn.forceActiveFocus()
            } else {
                moreBtn.forceActiveFocus()
            }
        }
    }

    // Focus the first visible button in header for Tab navigation
    function focusFirstButton() {
        if (collapseBtn.visible) {
            collapseBtn.forceActiveFocus()
        } else {
            moreBtn.forceActiveFocus()
        }
    }

    // Focus the last button in header for Shift+Tab navigation
    function focusLastButton() {
        clearAllBtn.forceActiveFocus()
    }

    RowLayout {
        anchors.fill: parent
        NotifyHeaderTitleText {
            text: qsTr("Notification Center")
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 18
            tFont: DTK.fontManager.t4
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        AnimationSettingButton {
            id: collapseBtn
            objectName: "collapse"
            visible: !notifyModel.collapse
            Layout.alignment: Qt.AlignRight
            activeFocusOnTab: false
            focusBorderVisible: activeFocus
            icon.name: "fold"
            text: qsTr("Fold")
            Keys.onTabPressed: function(event) {
                moreBtn.forceActiveFocus()
                event.accepted = true
            }
            Keys.onBacktabPressed: function(event) {
                root.gotoLastNotify()
                event.accepted = true
            }
            onClicked: function () {
                console.log("Collapse all notify")
                notifyModel.collapseAllApp()
            }
        }

        AnimationSettingButton {
            id: moreBtn
            objectName: "more"
            Layout.alignment: Qt.AlignRight
            activeFocusOnTab: false
            focusBorderVisible: activeFocus
            icon.name: "more"
            text: qsTr("More")
            Keys.onTabPressed: function(event) {
                clearAllBtn.forceActiveFocus()
                event.accepted = true
            }
            Keys.onBacktabPressed: function(event) {
                if (collapseBtn.visible) {
                    collapseBtn.forceActiveFocus()
                } else {
                    root.gotoLastNotify()
                }
                event.accepted = true
            }
            onClicked: function () {
                console.log("Notify setting")
                NotifyAccessor.openNotificationSetting()
            }
        }

        AnimationSettingButton {
            id: clearAllBtn
            objectName: "closeAllNotify"
            activeFocusOnTab: false
            focusBorderVisible: activeFocus
            icon.name: "clean-all"
            text: qsTr("Clear All")
            Layout.alignment: Qt.AlignRight
            Keys.onTabPressed: function(event) {
                clearAllBtn.focus = false  // Clear focus before signal to prevent focus state residue
                root.gotoFirstNotify()
                event.accepted = true
            }
            Keys.onBacktabPressed: function(event) {
                moreBtn.forceActiveFocus()
                event.accepted = true
            }
            onClicked: function () {
                console.log("Clear all notify")
                notifyModel.clear()
            }
        }
    }
}
