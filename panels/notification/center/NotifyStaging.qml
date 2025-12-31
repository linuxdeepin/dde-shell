// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    implicitWidth: 360
    implicitHeight: view.height
    property var model: notifyModel
    readonly property int viewCount: view.count

    signal gotoHeaderFirst()  // Signal to Tab to header first button
    signal gotoHeaderLast()   // Signal to Shift+Tab to header last button

    // Focus the first item in staging for Tab cycle
    function focusFirstItem() {
        if (view.count > 0) {
            view.currentIndex = 0
            Qt.callLater(function() {
                let firstItem = view.itemAtIndex(0)
                if (firstItem && firstItem.enabled) {
                    firstItem.forceActiveFocus()
                }
            })
        }
    }

    // Focus the last button in staging for Shift+Tab from header
    function focusLastButton() {
        if (view.count > 0) {
            let lastItem = view.itemAtIndex(view.count - 1)
            if (lastItem) {
                lastItem.forceActiveFocus()
                Qt.callLater(function() {
                    if (lastItem.notifyContent) {
                        lastItem.notifyContent.focusLastButton()
                    }
                })
            }
        }
    }

    NotifyStagingModel {
        id: notifyModel
    }

    // Navigate to next item or go to header
    function navigateToNextItem(currentIndex) {
        if (currentIndex < view.count - 1) {
            view.currentIndex = currentIndex + 1
            Qt.callLater(function() {
                let nextItem = view.itemAtIndex(currentIndex + 1)
                if (nextItem && nextItem.enabled) nextItem.forceActiveFocus()
            })
        } else {
            // Last item, go to header
            root.gotoHeaderFirst()
        }
    }

    // Navigate to previous item or go to header
    function navigateToPrevItem(currentIndex) {
        if (currentIndex > 0) {
            view.currentIndex = currentIndex - 1
            Qt.callLater(function() {
                let prevItem = view.itemAtIndex(currentIndex - 1)
                if (prevItem && prevItem.enabled) prevItem.forceActiveFocus()
            })
        } else {
            // First item, go to header last button
            root.gotoHeaderLast()
        }
    }

    ListView {
        id: view
        spacing: 10
        snapMode: ListView.SnapToItem
        width: root.width
        height: contentHeight

        model: notifyModel
        delegate: OverlapNotify {
            id: overlapNotify
            objectName: "overlap-" + model.appName
            focus: true
            width: 360
            activeFocusOnTab: true

            count: model.overlapCount
            appName: model.appName
            iconName: model.iconName
            date: model.time
            actions: model.actions
            defaultAction: model.defaultAction
            title: model.title
            content: model.content
            strongInteractive: model.strongInteractive
            contentIcon: model.contentIcon
            contentRowCount: model.contentRowCount

            // Tab key navigation: focus internal buttons first
            Keys.onTabPressed: function(event) {
                if (overlapNotify.focusFirstButton()) {
                    event.accepted = true
                    return
                }
                Qt.callLater(function() {
                    if (overlapNotify.focusFirstButton()) return
                    root.navigateToNextItem(index)
                })
                event.accepted = true
            }
            Keys.onBacktabPressed: function(event) {
                root.navigateToPrevItem(index)
                event.accepted = true
            }
            onGotoNextItem: root.navigateToNextItem(index)
            onGotoPrevItem: overlapNotify.forceActiveFocus()

            onRemove: function () {
                console.log("remove overlap", model.id)
                notifyModel.closeNotify(model.id, NotifyItem.Closed)
            }
            onDismiss: function () {
                console.log("dismiss overlap", model.id)
                notifyModel.closeNotify(model.id, NotifyItem.Dismissed)
            }
            onActionInvoked: function (actionId) {
                console.log("action overlap", model.id, actionId)
                notifyModel.invokeNotify(model.id, actionId)
            }
        }
        displaced: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "y"; duration: 300 }
            }
        }
    }
}

