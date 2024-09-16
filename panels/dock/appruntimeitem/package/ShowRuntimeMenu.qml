// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

ApplicationWindow {
    visible: true
    width: 800
    height: 400
    title: "应用监控"
    property var windowManagerModel

    Component.onCompleted: {
        windowManagerModel = windowManager; // 假设 windowManager 是 WindowManager 实例
    }

    Column {
        anchors.fill: parent

        // 前端应用表头
        Row {
            spacing: 10
            width: parent.width
            height: 40

            Text { text: "应用名称"; width: parent.width / 2 }
            Text { text: "窗口id"; width: parent.width / 4}
            Text { text: "运行时长"; width: parent.width / 4 }
        }

        ListView {
            id: listView
            anchors.fill: parent;
            anchors.topMargin: 30
            model: windowManagerModel
            delegate: Item {
                width: listView.width
                height: 30
                property var startTime: model.startTime
                Timer {
                    id: timer
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        var now = new Date();
                        var elapsedTime = now - startTime;

                        var hours = Math.floor(elapsedTime / (1000 * 60 * 60));
                        elapsedTime %= (1000 * 60 * 60);

                        var minutes = Math.floor(elapsedTime / (1000 * 60));
                        elapsedTime %= (1000 * 60);

                        var seconds = Math.floor(elapsedTime / 1000);

                        var displayText = "";
                        displayText += (hours < 10 ? "0" + hours : hours) + "h:";
                        displayText += (minutes < 10 ? "0" + minutes : minutes) + "m:";
                        displayText += (seconds < 10 ? "0" + seconds : seconds) + "s";

                        elapsedTimeText.text = displayText;
                    }
                }
                Row {
                    width: parent.width
                    height: parent.height
                    spacing: 10

                    Text {
                        text: name  // 使用模型角色
                        width: listView.width / 2
                    }
                    Text {
                        text: id // 使用模型角色
                        width: listView.width / 4
                    }
                    Text {
                        id: elapsedTimeText
                        text: "00h:00m:00s"
                        width: listView.width / 4

                    }
                }
            }
        }
    }
}
