// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
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
        spacing: 8
        // Title container with fade-out effect when compressed by buttons
        Item {
            id: titleContainer
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 18
            Layout.fillWidth: true
            implicitHeight: titleText.implicitHeight

            // Enable opacity mask only when text overflows container
            layer.enabled: titleText.implicitWidth > titleContainer.width + 5
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: titleContainer.width
                    height: titleContainer.height
                    // Horizontal gradient: fully visible on left, fade to transparent on right
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#FFFFFFFF" }  // Fully opaque
                        GradientStop { position: 0.9; color: "#FFFFFFFF" }  // Still opaque at 90%
                        GradientStop { position: 1.0; color: "#00FFFFFF" }  // Fully transparent
                    }
                }
            }

            NotifyHeaderTitleText {
                id: titleText
                text: qsTr("Notification Center")
                elide: Text.ElideNone
                tFont: DTK.fontManager.t4
            }
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
                console.log("Notify setting menu")
                headerSettingMenu.x = moreBtn.width / 2 - headerSettingMenu.width / 2
                headerSettingMenu.y = moreBtn.height
                headerSettingMenu.toggle()
            }

            NotifySettingMenu {
                id: headerSettingMenu
                MenuItem {
                    text: qsTr("Notification Setting")
                    onClicked: {
                        console.log("Notification setting")
                        NotifyAccessor.openNotificationSetting()
                    }
                }
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
