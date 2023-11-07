// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: taskmanager
    implicitWidth: 40
    implicitHeight: 40

    property bool useColumnLayout: dock.useColumnLayout

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
                id: contents
                anchors.fill: parent
                model: Applet.dataModel
                delegate: ItemDelegate {
                    required property string itemId
                    required property string name
                    required property string iconName
                    required property string menus
                    width: 40
                    height: 40
                    icon.name: iconName
                    icon.source: iconName
                    icon.width: 40
                    icon.height: 40
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton)
                                contextMenu.popup()
                            else
                                Applet.clickItem(itemId)
                        }
                    }

                    Menu {
                        id: contextMenu
                        height: 200
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                        Repeater {
                            model: JSON.parse(menus)
                            MenuItem {
                                text: modelData.name
                                height: 20
                                onTriggered: {
                                    console.log(itemId, modelData.id)
                                    taskmanager.Applet.clickItemMenu(itemId, modelData.id)
                                }
                            }
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
                id: contents
                anchors.fill: parent
                model: Applet.dataModel
                delegate: ItemDelegate {
                    required property string itemId
                    required property string name
                    required property string iconName
                    required property string menus
                    width: 40
                    height: 40
                    icon.name: iconName
                    icon.source: iconName
                    icon.width: 40
                    icon.height: 40
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton)
                                contextMenu.popup()
                            else
                                Applet.clickItem(itemId)
                        }
                    }

                    Menu {
                        id: contextMenu
                        height: 200
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                        Repeater {
                            model: JSON.parse(menus)
                            MenuItem {
                                text: modelData.name
                                height: 20
                                onTriggered: {
                                    console.log(itemId, modelData.id)
                                    taskmanager.Applet.clickItemMenu(itemId, modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
