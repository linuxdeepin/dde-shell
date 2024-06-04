// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0

Control {
    id: root
    property alias shellSurface: surfaceLayer.shellSurface
    property alias clickedEnabled: clickedLayer.enabled
    signal clicked()

    TapHandler {
        id: clickedLayer
        gesturePolicy: TapHandler.ReleaseWithinBounds
        acceptedButtons: Qt.LeftButton
        onTapped: {
            root.clicked()
        }
    }

    DragItem {
        id: dragLayer
        anchors.fill: parent
        dragItem: root
    }

    ShellSurfaceItem {
        id: surfaceLayer
        width: 100
        height: parent.height
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            opacity: 0.3
            color: "gray"
        }
    }

    background: BoxPanel {
        color2: color1
        radius: 18
    }
}
