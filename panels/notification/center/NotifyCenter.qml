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

    palette: DTK.palette

    property alias model: notifyModel
    property alias viewPanelShown: view.viewPanelShown
    property int maxViewHeight: 400
    property int stagingViewCount: 0
    readonly property int viewCount: view.viewCount

    signal gotoStagingLast()  // Signal to Shift+Tab to staging last button
    signal gotoStagingFirst() // Signal to Tab cycle to staging first item

    // Focus header first button (for Tab from staging)
    function focusHeaderFirst() {
        header.focusFirstButton()
    }

    // Focus header last button (for Shift+Tab from staging)
    function focusHeaderLast() {
        header.focusLastButton()
    }

    // Focus view last item (for Shift+Tab when no staging)
    function focusViewLastItem() {
        view.focusLastItem()
    }

    NotifyModel {
        id: notifyModel
    }

    Item {
        objectName: "notificationCenter"
        anchors.fill: parent
        NotifyHeader {
            id: header
            anchors {
                top: parent.top
                left: parent.left
                leftMargin: NotifyStyle.leftMargin
            }
            height: 40
            width: NotifyStyle.contentItem.width
            notifyModel: notifyModel
            z: 1
            onGotoFirstNotify: {
                if (view.viewCount === 0 || !view.focusItemAtIndex(0)) header.focusFirstButton()
            }
            onGotoLastNotify: {
                // First try to go to staging area if it has items
                if (root.stagingViewCount > 0) {
                    root.gotoStagingLast()
                } else if (view.viewCount === 0) {
                    header.focusLastButton()
                } else {
                    view.focusLastItem()
                }
            }
        }

        NotifyView {
            id: view
            anchors {
                left: parent.left
                top: header.bottom
                right: parent.right
                rightMargin: NotifyStyle.scrollBarPadding
                bottom: parent.bottom
            }

            height: Math.min(maxViewHeight, viewHeight)
            notifyModel: notifyModel
            onGotoHeaderFirst: {
                // If staging has items, go to staging first; otherwise go to header
                if (root.stagingViewCount > 0) {
                    root.gotoStagingFirst()
                } else {
                    header.focusFirstButton()
                }
            }
            onGotoHeaderLast: header.focusLastButton()
        }

        DropShadowText {
            text: qsTr("No recent notifications")
            visible: root.stagingViewCount === 0 && view.viewCount === 0
            anchors {
                top: header.bottom
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }
            height: 40
        }
    }
}
