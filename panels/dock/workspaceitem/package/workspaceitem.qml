// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: workspaceitem
    property int dockSize: Applet.parent.dockSize
    property int dockOrder: 2
    property int frameSize: 40
    property int itemSize: 20
    implicitWidth: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? model.count * frameSize : dockSize
    implicitHeight: Panel.position === Dock.Left || Panel.position === Dock.Right ? model.count * frameSize : dockSize

    component WorkspaceDelegate: Item {
        id: content
        required property int index
        required property string name
        required property string screenImage
        property bool isCurrent: content.ListView.view.currentIndex === content.index

        implicitWidth: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? frameSize : dockSize
        implicitHeight: Panel.position === Dock.Left || Panel.position === Dock.Right ? frameSize : dockSize
        anchors.verticalCenter: parent

        Rectangle {
            anchors.centerIn: parent
            border.width: 1
            implicitWidth: isCurrent ? itemSize + 5 : itemSize
            implicitHeight: isCurrent ? itemSize + 5 : itemSize
            color: "transparent"
            // todo : workspace image
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    content.ListView.view.currentIndex = content.index
                    console.info("index = " + content.ListView.view.currentIndex)
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        ListView {
            id: listView
            anchors.fill: parent
            spacing: 0
            orientation: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? ListView.Horizontal : ListView.Vertical
            delegate: WorkspaceDelegate {}

            // todo : data
            model: ListModel {
                id: model
                property int currentIndex: 1
                ListElement {
                    name: qsTr("Screen1")
                    screenImage: qsTr("1")
                }
                ListElement {
                    name: qsTr("Screen2")
                    screenImage: qsTr("2")
                }
                ListElement {
                    name: qsTr("Screen3")
                    screenImage: qsTr("3")
                }
                ListElement {
                    name: qsTr("Screen4")
                    screenImage: qsTr("4")
            }
        }
    }
}
