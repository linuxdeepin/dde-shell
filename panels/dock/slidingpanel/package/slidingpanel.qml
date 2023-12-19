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
    // property int dockOrder: Panel.displayMode == Dock.Efficient ? 27 : 1
    // TODO: not ready, so mark -1,
    property int dockOrder: -1
    implicitWidth: useColumnLayout ? dockSize: content.implicitWidth
    implicitHeight: useColumnLayout ? content.implicitHeight : dockSize

    Control {
        id: content
        anchors.fill: parent
        padding: 4

        contentItem: SwipeView {
            id: listView
            orientation: useColumnLayout ? ListView.Horizontal : ListView.Vertical
            implicitWidth: useColumnLayout ? dockSize - 10 : 100
            implicitHeight: useColumnLayout ? 100 : dockSize - 10
            clip: true

            // test data
            Repeater {
                model: ListModel {
                    ListElement { name: "Element 1" }
                    ListElement { name: "Element 2" }
                    ListElement { name: "Element 3" }
                    ListElement { name: "Element 4" }
                }

                delegate: Rectangle {
                    implicitWidth: slidingpanel.useColumnLayout ? dockSize - 10: 100
                    implicitHeight: slidingpanel.useColumnLayout ? 100 : dockSize - 10
                    color: "red"
                    radius: (dockSize / 2.8) - 5

                    Text {
                        text: model.name
                        font.pixelSize: 20
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }
}
