// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: taskmanager
    implicitWidth: grid.implicitWidth
    implicitHeight: grid.implicitHeight

    property bool useColumnLayout: Applet.parent.position % 2

    Grid {
        id: grid
        flow: useColumnLayout ? Grid.TopToBottom : Grid.LeftToRight
        Repeater {
            id: contents
            model: Applet.dataModel
            delegate: AppItem {
                displayMode: Applet.parent.displayMode
                colorTheme: Applet.parent.colorTheme
                Component.onCompleted: {
                    clickItem.connect(Applet.clickItem)
                    clickItemMenu.connect(Applet.clickItemMenu)
                }
            }
        }
    }


    // Loader {
    //     id: contentLoader
    //     sourceComponent: useColumnLayout ? columnComponent : rowComponent
    // }

    // Component {
    //     id: rowComponent
    //     Row {
    //         id: rowLayout
    //         Repeater {
    //             id: contents
    //             model: Applet.dataModel
    //             delegate: AppItem {
    //                 displayMode: dock.displayMode
    //                 colorTheme: dock.colorTheme
    //                 Component.onCompleted: {
    //                     clickItem.connect(Applet.clickItem)
    //                     clickItemMenu.connect(Applet.clickItemMenu)
    //                 }
    //             }
    //         }
    //     }
    // }

    // Component {
    //     id: columnComponent
    //     Column {
    //         id: columnLayout
    //         Repeater {
    //             id: contents
    //             model: Applet.dataModel
    //             delegate: AppItem {
    //                 displayMode: dock.displayMode
    //                 colorTheme: dock.colorTheme
    //                 Component.onCompleted: {
    //                     clickItem.connect(Applet.clickItem)
    //                     clickItemMenu.connect(Applet.clickItemMenu)
    //                 }
    //             }
    //         }
    //     }
    // }
}
