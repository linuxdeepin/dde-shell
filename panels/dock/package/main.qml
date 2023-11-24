// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Window 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

Window {
    id: dock
    visible: true
    width:  useColumnLayout ? Applet.dockSize : Screen.width
    height: useColumnLayout ? Screen.height : Applet.dockSize

    D.DWindow.enabled: true
    DLayerShellWindow.anchors: position2Anchors(Applet.position)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.exclusionZone: Applet.dockSize

    property bool useColumnLayout: Applet.position % 2
    
    Loader {
        id: contentLoader
        anchors.fill: parent
        sourceComponent: useColumnLayout ? columnComponent : rowComponent
    }

    Component {
        id: rowComponent
        Row {
            id: rowLayout
            anchors.fill: parent
            Repeater {
                anchors.fill: parent
                model: Applet.appletItems
                delegate: Loader {
                    sourceComponent: Control {
                        contentItem: model.modelData
                        horizontalPadding: 5
                        Component.onCompleted: {
                            contentItem.parent = this
                        }
                    }
                }
            }
        }
    }

    Component {
        id: columnComponent
        Column {
            id: columnLayout
            anchors.fill: parent
            Repeater {
                anchors.fill: parent
                model: Applet.appletItems
                delegate: Loader {
                    sourceComponent: Control {
                        contentItem: model.modelData
                        verticalPadding: 5
                        Component.onCompleted: {
                            contentItem.parent = this
                        }
                    }
                }
            }
        }
    }

    function position2Anchors(position) {
        if (position === 0)
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop
        if (position === 1)
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorRight
        if (position === 2)
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorBottom
        if (position === 3)
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorLeft
    }

    // Component.onCompleted: {
    //     Applet.onPositionChanged.connect(function() {
    //         dock.useColumnLayout = Applet.position % 2
    //         contentLoader.sourceComponent = undefined
    //         contentLoader.sourceComponent = dock.useColumnLayout ? columnComponent : rowComponent
    //     })
    // }
}
