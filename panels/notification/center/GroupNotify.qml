// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root
    implicitWidth: impl.implicitWidth
    implicitHeight: impl.implicitHeight

    signal collapse()
    signal gotoNextItem()  // Signal to navigate to next notify item
    signal gotoPrevItem()  // Signal to navigate to previous notify item

    // Focus the first button for Tab navigation into group
    function focusFirstButton() {
        foldBtn.forceActiveFocus()
        return true
    }

    Control {
        id: impl
        anchors.fill: parent

        contentItem: RowLayout {
            NotifyHeaderTitleText {
                text: root.appName
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 18
                tFont: DTK.fontManager.t5
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            AnimationSettingButton {
                id: foldBtn
                Layout.alignment: Qt.AlignRight
                activeFocusOnTab: false
                focusBorderVisible: activeFocus
                icon.name: "fold"
                text: qsTr("Fold")
                Keys.onTabPressed: function(event) {
                    groupMoreBtn.forceActiveFocus()
                    event.accepted = true
                }
                Keys.onBacktabPressed: function(event) {
                    root.gotoPrevItem()
                    event.accepted = true
                }
                onClicked: {
                    console.log("collapse")
                    root.collapse()
                }
            }
            AnimationSettingButton {
                id: groupMoreBtn
                Layout.alignment: Qt.AlignRight
                activeFocusOnTab: false
                focusBorderVisible: activeFocus
                icon.name: "more"
                text: qsTr("More")
                Keys.onTabPressed: function(event) {
                    groupClearBtn.forceActiveFocus()
                    event.accepted = true
                }
                Keys.onBacktabPressed: function(event) {
                    foldBtn.forceActiveFocus()
                    event.accepted = true
                }
                onClicked: function () {
                    console.log("group setting", root.appName)
                    let pos = mapToItem(root, Qt.point(width / 2, height))
                    root.setting(pos)
                }
            }
            AnimationSettingButton {
                id: groupClearBtn
                Layout.alignment: Qt.AlignRight
                activeFocusOnTab: false
                focusBorderVisible: activeFocus
                icon.name: "clean-group"
                text: qsTr("Clear All")
                Keys.onTabPressed: function(event) {
                    groupClearBtn.focus = false  // Clear focus before signal to prevent focus state residue
                    root.gotoNextItem()
                    event.accepted = true
                }
                Keys.onBacktabPressed: function(event) {
                    groupMoreBtn.forceActiveFocus()
                    event.accepted = true
                }
                onClicked: function () {
                    root.remove()
                }
            }
        }
    }
}
