// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQml
import QtWayland.Compositor

import org.deepin.ds 1.0
import org.deepin.dtk 1.0
import org.deepin.ds.dock 1.0

Control {
    id: root
    required property var shellSurface
    signal clicked()
    contentItem: RowLayout {
        TapHandler {
            id: clickedLayer
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.clicked()
            }
        }

        Loader {
            active: root.shellSurface
            visible: active
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            sourceComponent: ShellSurfaceItem {
                shellSurface: root.shellSurface
            }
        }

        DciIcon {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            name: "dock-control-panel"
        }
    }
    background: BoxPanel {
        color2: color1
        radius: 18
    }
}
