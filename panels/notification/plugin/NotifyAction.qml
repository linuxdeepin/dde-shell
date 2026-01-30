// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.dtk.private 1.0 as DP
import org.deepin.ds.notification

Control {
    id: root

    property var actions: []
    signal actionInvoked(var actionId)
    signal gotoNextButton()  // Signal to Tab to next button (X button or next notify)
    signal gotoPrevItem()    // Signal to Shift+Tab to previous notify item

    // Focus the first action button for Tab navigation
    function focusFirstButton() {
        if (actions.length > 0) {
            firstActionBtn.forceActiveFocus()
        }
    }

    // Focus the last action button for Shift+Tab navigation
    function focusLastButton() {
        if (actions.length === 2 && secondActionLoader.item) {
            secondActionLoader.item.forceActiveFocus()
        } else if (actions.length > 2 && moreActionsLoader.item) {
            moreActionsLoader.item.forceActiveFocus()
        } else if (actions.length > 0) {
            firstActionBtn.forceActiveFocus()
        }
    }

    contentItem: RowLayout {
        spacing: 5
        height: 30
        NotifyActionButton {
            id: firstActionBtn
            actionData: actions.length > 0 ? actions[0] : null
            visible: actions.length > 0
            activeFocusOnTab: false
            Layout.maximumWidth: 120
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            Keys.onBacktabPressed: function(event) {
                root.gotoPrevItem()
                event.accepted = true
            }
            Keys.onTabPressed: function(event) {
                if (actions.length === 1) {
                    root.gotoNextButton()
                } else if (actions.length === 2 && secondActionLoader.item) {
                    secondActionLoader.item.forceActiveFocus()
                } else if (actions.length > 2 && moreActionsLoader.item) {
                    moreActionsLoader.item.forceActiveFocus()
                }
                event.accepted = true
            }
        }

        Loader {
            id: secondActionLoader
            active: actions.length === 2
            visible: active
            Layout.maximumWidth: 120
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            sourceComponent: NotifyActionButton {
                actionData: actions.length > 1 ? actions[1] : null
                activeFocusOnTab: false
                Keys.onBacktabPressed: function(event) {
                    firstActionBtn.forceActiveFocus()
                    event.accepted = true
                }
                Keys.onTabPressed: function(event) {
                    root.gotoNextButton()
                    event.accepted = true
                }
            }
        }

        Loader {
            id: moreActionsLoader
            active: actions.length > 2
            visible: active
            Layout.maximumWidth: 200
            Layout.alignment: Qt.AlignHCenter
            sourceComponent: ComboBox {
                property var expandActions: actions.slice(1)
                textRole: "text"
                implicitHeight: 30
                implicitWidth: 160
                model: expandActions
                activeFocusOnTab: false
                Keys.onBacktabPressed: function(event) {
                    firstActionBtn.forceActiveFocus()
                    event.accepted = true
                }
                Keys.onTabPressed: function(event) {
                    root.gotoNextButton()
                    event.accepted = true
                }
                delegate: NotifyActionButton {
                    required property int index
                    width: parent.width
                    actionData: expandActions[index]
                    activeFocusOnTab: false
                }
            }
        }
    }

    component NotifyActionButton: Button {
        id: actionButton
        required property var actionData
        text: actionData ? actionData.text : ""
        enabled: actionData ? actionData.enabled : false
        topPadding: undefined
        bottomPadding: undefined
        leftPadding: undefined
        rightPadding: undefined
        padding: 6
        spacing: 0
        font: DTK.fontManager.t6
        onClicked: {
            if (actionData) {
                console.log("action invoked", actionData.id)
                actionInvoked(actionData.id)
            }
        }

        Loader {
            anchors.fill: parent
            active: actionButton.activeFocus

            sourceComponent: FocusBoxBorder {
                radius: 6
                color: palette.highlight
            }
        }

        background: NotifyItemBackground {
            implicitHeight: 30
            implicitWidth: 50
            radius: 6
            outsideBorderColor: null
            insideBorderColor: null
            anchors.fill: parent
        }
    }
}
