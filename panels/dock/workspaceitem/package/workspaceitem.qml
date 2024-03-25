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
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 2
    property int frameSize: 20
    property int itemSize: 16
    property int space: 4
    // todo: visible property to be set
    property bool shouldVisible: listView.count > 1
    // visible:listView.count > 1
    implicitWidth: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? listView.count * frameSize + space * listView.count : dockSize
    implicitHeight: Panel.position === Dock.Left || Panel.position === Dock.Right ? listView.count * frameSize + space * listView.count : dockSize

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onHoveredChanged: {
            Applet.dataModel.preview(containsMouse)
        }
    }

    component WorkspaceDelegate: Item {
        id: content
        required property int index
        required property string screenImage
        property bool isCurrent: content.ListView.view.currentIndex === content.index

        implicitWidth: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? frameSize : dockSize
        implicitHeight: Panel.position === Dock.Left || Panel.position === Dock.Right ? frameSize : dockSize

        Rectangle {
            anchors.centerIn: parent
            border.width: 1
            border.color: isCurrent ? "black" : Qt.rgba(0, 0, 0, 0.5)
            implicitWidth: frameSize
            implicitHeight: isCurrent ? itemSize + 4 : itemSize
            color: Qt.rgba(0, 0, 0, 0.1)
            Image {
                anchors.fill: parent
                anchors.margins: 1
                id: workspaceImage
                source: screenImage
                visible: isCurrent
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    Applet.dataModel.currentIndex =  content.index
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
            spacing: 4
            orientation: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? ListView.Horizontal : ListView.Vertical
            delegate: WorkspaceDelegate{}

            model: Applet.dataModel
            currentIndex: Applet.dataModel.currentIndex
        }
    }
}
