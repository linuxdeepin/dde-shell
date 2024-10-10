// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

D.Control {
    id: control
    property var bubble

    contentItem: RowLayout {
        Layout.minimumHeight: 40

        ColumnLayout {
            Layout.topMargin: 8
            Layout.leftMargin: 8

            D.DciIcon {
                sourceSize: Qt.size(24, 24)
                name: bubble.iconName
            }

            Item {
                Layout.fillHeight: true
            }
        }

        ColumnLayout {
            spacing: 0
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.rightMargin: 6
            Layout.fillWidth: true
            Layout.fillHeight: true

            // The first line: appName and timeTip
            RowLayout {
                Text {
                    visible: bubble.appName
                    Layout.alignment: Qt.AlignLeft
                    elide: Text.ElideRight
                    text: bubble.appName
                    Layout.minimumWidth: 0
                    maximumLineCount: 1
                    font: D.DTK.fontManager.t9
                    color: D.DTK.themeType === D.ApplicationHelper.DarkType ?
                        Qt.rgba(1, 1, 1, 0.6) : Qt.rgba(0, 0, 0, 0.6)
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    id: timeTip
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 5
                    visible: !control.hovered
                    elide: Text.ElideRight
                    text: bubble.timeTip
                    maximumLineCount: 1
                    font: D.DTK.fontManager.t9
                    color: D.DTK.themeType === D.ApplicationHelper.DarkType ?
                        Qt.rgba(1, 1, 1, 0.6) : Qt.rgba(0, 0, 0, 0.6)
                }
            }

            // The second line: summary
            Text {
                visible: bubble.summary !== ""
                Layout.alignment: Qt.AlignLeft
                Layout.rightMargin: 5
                elide: Text.ElideRight
                text: bubble.summary
                Layout.fillWidth: true
                maximumLineCount: 1
                font: D.DTK.fontManager.t8
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }

            // The third line: body and image
            RowLayout {
                Text {
                    visible: bubble.body !== ""
                    Layout.alignment: Qt.AlignLeft
                    Layout.rightMargin: 5
                    elide: Text.ElideRight
                    text: bubble.body
                    Layout.fillWidth: true
                    maximumLineCount: 6
                    font: D.DTK.fontManager.t8
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                Image {
                    visible: bubble.bodyImagePath !== ""
                    source: bubble.bodyImagePath
                    Layout.alignment: Qt.AlignVCenter

                    fillMode: Image.PreserveAspectFit
                }
            }

            // single line action buttons
            Loader {
                Layout.topMargin: 6
                Layout.rightMargin: 8
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                active: bubble.hasDisplayAction && bubble.urgency === 2
                sourceComponent: BubbleAction {
                    bubble: control.bubble
                    onActionInvoked: function(actionId) {
                        console.log("action", actionId, bubble.index)
                        Applet.invokeAction(bubble.index, actionId)
                    }
                }
            }
        }
    }

    // hover action buttons
    Loader {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        active: control.hovered && bubble.hasDisplayAction && bubble.urgency !== 2
        sourceComponent: BubbleAction {
            bubble: control.bubble
            onActionInvoked: function(actionId) {
                console.log("action", actionId, bubble.index)
                Applet.invokeAction(bubble.index, actionId)
            }
        }
    }

    Button {
         id: closeBtn
         visible: control.hovered
         width: 20
         height: 20
         icon.name: "window-close"
         icon.width: 10
         icon.height: 10
         anchors.right: parent.right
         anchors.top: parent.top
         anchors.topMargin: -7
         anchors.rightMargin: -7
         z: control.z + 1
         onClicked: Applet.delayProcess(bubble.index)
    }
}