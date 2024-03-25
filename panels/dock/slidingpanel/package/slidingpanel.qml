// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: slidingpanel
    property int dockSize: Panel.rootObject.dockSize
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 27

    implicitWidth: Math.max(dockSize, 70)
    implicitHeight: dockSize
    Rectangle {
        anchors.fill: parent
        implicitWidth: slidingpanel.useColumnLayout ? dockSize - 10 : timedate.implicitWidth
        implicitHeight: slidingpanel.useColumnLayout ? timedate.implicitHeight : dockSize - 10
        color: "transparent"

        Column {
            id: timedate
            anchors.centerIn: parent

            property var currentTime: new Date()
            property var weekDay: ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]

            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: {
                    timedate.currentTime = new Date();
                }
            }

            Text {
                id: timeText
                anchors.horizontalCenter: parent.horizontalCenter

                width: slidingpanel.useColumnLayout ? dockSize - 10 : undefined
                fontSizeMode: Text.Fit
                wrapMode: Text.WordWrap

                text: {
                    "<b>" + Qt.formatTime(timedate.currentTime, "hh:mm") + "</b>"
                }
            }

            Text {
                id: dateText
                anchors.horizontalCenter: parent.horizontalCenter

                width: slidingpanel.useColumnLayout ? dockSize - 10 : undefined
                fontSizeMode: Text.Fit
                wrapMode: Text.WordWrap

                text: {
                    Qt.formatDate(timedate.currentTime, "MM/dd") + " " + timedate.weekDay[timedate.currentTime.getDay()]
                }
            }
        }
    }
}
