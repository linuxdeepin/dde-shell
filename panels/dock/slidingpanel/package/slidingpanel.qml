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
    property int dockSize: Panel.dockSize
    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: Panel.displayMode == Dock.Efficient ? 27 : 1

    implicitWidth: useColumnLayout ? dockSize: content.implicitWidth
    implicitHeight: useColumnLayout ? content.implicitHeight : dockSize

    Control {
        id: content
        anchors.fill: parent
        
        leftPadding: {
            if (Panel.displayMode === Dock.Efficient) return 2
            return slidingpanel.useColumnLayout ? 4 : 10
        }
        rightPadding: {
            if (Panel.displayMode === Dock.Efficient) return 2
            return slidingpanel.useColumnLayout ? 4 : 0
        }

        topPadding: {
            if (Panel.displayMode === Dock.Efficient) return 2
            return slidingpanel.useColumnLayout ? 10 : 4
        }
        bottomPadding: {
            if (Panel.displayMode === Dock.Efficient) return 2
            return slidingpanel.useColumnLayout ? 0 : 4
        }

        contentItem: Rectangle {
            id: listView

            implicitWidth: slidingpanel.useColumnLayout ? dockSize - 10 : timedate.implicitWidth
            implicitHeight: slidingpanel.useColumnLayout ? timedate.implicitHeight : dockSize - 10
            radius: (dockSize - 30) / 2

            Column {
                id: timedate
                anchors.fill: parent

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

    // contentItem: SwipeView {
    //         id: listView
    //         orientation: useColumnLayout ? ListView.Horizontal : ListView.Vertical
    //         implicitWidth: useColumnLayout ? dockSize - 10 : 100
    //         implicitHeight: useColumnLayout ? 100 : dockSize - 10
    //         clip: true

    //         // test data
    //         Repeater {
    //             model: ListModel {
    //                 ListElement { name: "Element 1" }
    //                 ListElement { name: "Element 2" }
    //                 ListElement { name: "Element 3" }
    //                 ListElement { name: "Element 4" }
    //             }

    //             delegate: Rectangle {
    //                 implicitWidth: slidingpanel.useColumnLayout ? dockSize - 10: 100
    //                 implicitHeight: slidingpanel.useColumnLayout ? 100 : dockSize - 10
    //                 color: "red"
    //                 radius: (dockSize / 2.8) - 5

    //                 Text {
    //                     text: model.name
    //                     font.pixelSize: 20
    //                     anchors.centerIn: parent
    //                 }
    //             }
    //         }
    //     }
    // }
}
