// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.dtk.style 1.0 as DStyle
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root

    property bool closeVisible: root.hovered || root.activeFocus
    // placeHolder to receive MouseEvent
    Control {
        id: closePlaceHolder
        focus: true
        anchors {
            top: parent.top
            topMargin: -height / 2
            right: parent.right
            rightMargin: -width / 2
        }
        contentItem: Loader {
            active: root.closeVisible || closePlaceHolder.hovered || closePlaceHolder.activeFocus || activeFocus
            sourceComponent: SettingActionButton {
                objectName: "closeNotiry-" + root.appName
                icon.name: "close"
                icon.width: 20
                icon.height: 20
                forcusBorderVisible: visualFocus || closePlaceHolder.visualFocus
                onClicked: function () {
                    root.remove()
                }
            }
        }
    }
    Loader {
        id: actionPlaceHolder
        anchors {
            bottom: parent.bottom
            bottomMargin: 10
            right: parent.right
            rightMargin: 10
        }

        active: !root.strongInteractive && root.actions.length > 0
        visible: active
        sourceComponent: NotifyAction {
            actions: root.actions
            onActionInvoked: function (actionId) {
                root.actionInvoked(actionId)
            }
        }
    }

    contentItem: RowLayout {
        spacing: 0
        DciIcon {
            name: root.iconName
            sourceSize: Qt.size(24, 24)
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.topMargin: 10
            Layout.leftMargin: 10
        }

        ColumnLayout {
            spacing: 0
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.rightMargin: 10
            Layout.leftMargin: 10
            Layout.topMargin: 4
            Layout.bottomMargin: 10
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            Layout.maximumHeight: 240
            RowLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Text {
                    text: root.appName
                    font: DTK.fontManager.t10
                }

                Item {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                }

                Loader {
                    id: time
                    active: !root.closeVisible
                    visible: active
                    Layout.alignment: Qt.AlignRight
                    sourceComponent: Text {
                        text: root.date
                        font: DTK.fontManager.t10
                    }
                }
            }

            Text {
                text: root.title
                visible: text !== ""
                maximumLineCount: 1
                font: DTK.fontManager.t3
                wrapMode: Text.NoWrap
                elide: Text.ElideMiddle
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Text {
                    text: root.content
                    visible: text !== ""
                    maximumLineCount: 6
                    font: DTK.fontManager.t5
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                }
                Item {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                }

                Loader {
                    Layout.maximumWidth: 106
                    Layout.maximumHeight: 106
                    Layout.minimumWidth: 16
                    Layout.minimumHeight: 16
                    Layout.alignment: Qt.AlignRight
                    // TODO DciIcon's bounding can't be limit by maximumWidth.
                    sourceComponent: Image {
                        anchors.fill: parent
                        source: root.contentIcon
                    }
                }
            }

            Loader {
                active: root.strongInteractive && root.actions.length > 0
                visible: active
                Layout.bottomMargin: 10
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                sourceComponent: NotifyAction {
                    actions: root.actions
                    onActionInvoked: function (actionId) {
                        root.actionInvoked(actionId)
                    }
                }
            }
        }
    }

    background: NotifyItemBackground {
        radius: root.radius
    }
}
