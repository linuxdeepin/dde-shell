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
    readonly property bool hasEnabledAction: {
        if (actions.length === 0) return false
        for (let i = 0; i < actions.length; i++) {
            if (actions[i] && actions[i].enabled) return true
        }
        return false
    }
    signal actionInvoked(var actionId)
    signal gotoNextButton()  // Signal to Tab to next button (X button or next notify)
    signal gotoPrevItem()    // Signal to Shift+Tab to previous notify item

    // Focus the first action button for Tab navigation
    // Returns true if an enabled button was found and focused
    function focusFirstButton() {
        if (actions.length > 0 && actions[0] && actions[0].enabled) {
            firstActionBtn.forceActiveFocus()
            return true
        }
        // If first action is disabled, try to find the first enabled action
        for (let i = 1; i < actions.length; i++) {
            if (actions[i] && actions[i].enabled) {
                if (i === 1 && secondActionLoader.item) {
                    secondActionLoader.item.forceActiveFocus()
                    return true
                } else if (i > 1 && moreActionsLoader.item) {
                    moreActionsLoader.item.forceActiveFocus()
                    return true
                }
            }
        }
        return false
    }

    // Focus the last action button for Shift+Tab navigation
    function focusLastButton() {
        // Find the last enabled action button
        for (let i = actions.length - 1; i >= 0; i--) {
            if (actions[i] && actions[i].enabled) {
                if (i === 1 && secondActionLoader.item) {
                    secondActionLoader.item.forceActiveFocus()
                    return true
                } else if (i > 1 && moreActionsLoader.item) {
                    moreActionsLoader.item.forceActiveFocus()
                    return true
                } else if (i === 0) {
                    firstActionBtn.forceActiveFocus()
                    return true
                }
            }
        }
        return false
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
                // Try to go to next action button or next notification item
                if (actions.length === 1) {
                    // Only one action button, go to next item
                    root.gotoNextButton()
                } else if (actions.length === 2) {
                    // Try to focus second action button
                    if (secondActionLoader.item && secondActionLoader.item.enabled) {
                        secondActionLoader.item.forceActiveFocus()
                    } else {
                        // Second button is disabled, go to next item
                        root.gotoNextButton()
                    }
                } else if (actions.length > 2) {
                    // Try to focus more actions combo
                    if (moreActionsLoader.item) {
                        moreActionsLoader.item.forceActiveFocus()
                    }
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
                    // Go back to first action button
                    if (firstActionBtn.enabled) {
                        firstActionBtn.forceActiveFocus()
                    } else {
                        // First button is disabled, go to previous item
                        root.gotoPrevItem()
                    }
                    event.accepted = true
                }
                Keys.onTabPressed: function(event) {
                    // Last action button, go to next item
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
                    // Go back to first action button
                    if (firstActionBtn.enabled) {
                        firstActionBtn.forceActiveFocus()
                    } else {
                        // First button is disabled, go to previous item
                        root.gotoPrevItem()
                    }
                    event.accepted = true
                }
                Keys.onTabPressed: function(event) {
                    // Last action button, go to next item
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
