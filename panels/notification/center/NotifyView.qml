// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds
import org.deepin.ds.notificationcenter

Control {
    id: root
    focus: true

    required property NotifyModel notifyModel
    property alias viewPanelShown: view.panelShown
    readonly property real viewHeight: view.contentHeight
    readonly property int viewCount: view.count

    NotifySetting {
        id: notifySetting
        notifyModel: root.notifyModel
    }

    contentItem: ListView {
        id: view
        spacing: 10
        snapMode: ListView.SnapToItem
        // activeFocusOnTab: true
        ScrollBar.vertical: ScrollBar { }
        property int nextIndex: -1
        property bool panelShown: false
        
        onNextIndexChanged: {
            if (nextIndex >= 0 && count > 0) {
                currentIndex = nextIndex
            }
        }

        model: root.notifyModel
        delegate: NotifyViewDelegate {
            id: notifyDelegate
            notifyModel: root.notifyModel
            view: view
            onSetting: function (pos, params) {
                let appName = params.appName
                let pinned = params.pinned
                notifySetting.x = pos.x - notifySetting.width / 2
                notifySetting.y = pos.y
                notifySetting.appName = appName
                notifySetting.pinned = pinned

                console.log("setting", appName, pinned)
                notifySetting.toggle();
            }
        }
        add: Transition {
            id: addTrans
            enabled: view.panelShown

            NumberAnimation { 
                properties: "y"
                from: {
                    if (addTrans.ViewTransition.item.objectName.startsWith("overlap-")) {
                        // 24: group notify overlap height
                        return addTrans.ViewTransition.destination.y + view.spacing + 24
                    } else if (addTrans.ViewTransition.item.indexInGroup === 0) {
                        return addTrans.ViewTransition.destination.y - view.spacing - 24
                    } else {
                        return addTrans.ViewTransition.destination.y
                    }
                }
                duration: 400
                easing.type: Easing.OutQuart
            }
            
            NumberAnimation {
                property: "opacity";
                from: {
                    if (addTrans.ViewTransition.item.objectName.startsWith("overlap-")) {
                        return 0
                    } else if (addTrans.ViewTransition.item.indexInGroup === 0) {
                        return 1
                    } else {
                        return 0
                    }
                }
                to: 1
                duration: {
                    if (addTrans.ViewTransition.item.objectName.startsWith("overlap-")) {
                        return 0
                    } else if (addTrans.ViewTransition.item.indexInGroup === 0) {
                        return 100
                    } else {
                        return 600
                    }
                }
                easing.type: Easing.OutExpo
            }
        }

        remove: Transition {
            id: removeTrans
            NumberAnimation {
                properties: "y"
                to: {
                    if (removeTrans.ViewTransition.item.objectName.startsWith("overlap-")) {
                        return removeTrans.ViewTransition.destination.y + view.spacing + 24
                    } else if (removeTrans.ViewTransition.item.indexInGroup === 1) {
                        return removeTrans.ViewTransition.destination.y - view.spacing - 24
                    } else if (removeTrans.ViewTransition.item.indexInGroup === 2) {
                        return removeTrans.ViewTransition.destination.y - view.spacing - 24
                    } else {
                        return removeTrans.ViewTransition.destination.y
                    }
                }
                duration: 400
                easing.type: Easing.OutQuart
            }

            NumberAnimation {
                property: "scale";
                from: 1
                to: {
                    if (removeTrans.ViewTransition.item.indexInGroup === 1) {
                        return 0.9
                    } else if (removeTrans.ViewTransition.item.indexInGroup === 2) {
                        return 0.9
                    } else {
                        return 1
                    }
                }
                duration: 600
                easing.type: Easing.OutExpo
            }

            NumberAnimation {
                property: "opacity";
                to: 0
                duration: {
                    if (removeTrans.ViewTransition.item.indexInGroup === 0) {
                        return 100
                    } else if (removeTrans.ViewTransition.item.objectName.startsWith("overlap-")) {
                        return 100
                    } else {
                        return 600
                    }
                }
                easing.type: Easing.OutExpo
            }
        }

        addDisplaced: Transition {
            id: addDisplacedTrans
            NumberAnimation { property: "y"; duration: 400; easing.type: Easing.OutQuart}
            NumberAnimation { property: "opacity"; to: 1.0; duration: 600;  easing.type: Easing.OutExpo}
        }

        removeDisplaced: Transition {
            id: removeDisplacedTrans
            NumberAnimation { properties: "y"; duration: 400; easing.type: Easing.OutQuart}
        }
    }

    background: BoundingRectangle {}
}
