// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle
import org.deepin.dtk.private 1.0 as DP
import org.deepin.ds.dock 1.0

import Qt.labs.platform 1.1 as LP

AppletItem {
    id: searchItem
    property int dockSize: Panel.rootObject.dockSize
    property int dockOrder: 3
    implicitWidth: Panel.rootObject.useColumnLayout ? dockSize : 30 
    implicitHeight: Panel.rootObject.useColumnLayout ? 30 : dockSize
    property bool shouldVisible: Applet.visible

    property D.Palette toolButtonColor: DockPalette.toolButtonColor
    property D.Palette toolButtonBorderColor: DockPalette.toolButtonBorderColor


    property D.Palette backgroundColor: D.Palette {
        normal {
            common: Qt.rgba(1, 1, 1, 0.4)
        }
        normalDark{
            common: Qt.rgba(1, 1, 1, 0.2)
        }
    }

    PanelToolTip {
        id: toolTip
        text: qsTr("GrandSearch")
    }

    D.ToolButton {
        id: button
        anchors.centerIn: parent
        width: 30
        height: 30
        icon.name: "search"
        icon.width: 16
        icon.height: 16
        display: D.IconLabel.IconOnly
        onClicked: {
            toolTip.close()
            Applet.toggleGrandSearch()
        }
        onHoveredChanged: {
            if (hovered) {
                var point = Applet.rootObject.mapToItem(null, Applet.rootObject.width / 2, 0)
                toolTip.toolTipX = Qt.binding(function () {
                    return point.x - toolTip.width / 2
                })
                toolTip.toolTipY = Qt.binding(function () {
                    return -toolTip.height - 10
                })
                toolTip.open()
            } else {
                toolTip.close()
            }
        }

        // TODO: get style from Dtk
        background: DP.ButtonPanel {
            button: button
            color1: searchItem.toolButtonColor
            color2: searchItem.toolButtonColor
            outsideBorderColor: searchItem.toolButtonBorderColor
            insideBorderColor: null
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            platformMenuLoader.active = true
            platformMenuLoader.item.open()
        }
    }

    Loader {
        id: platformMenuLoader
        active: false
        sourceComponent: LP.Menu {
            id: platformMenu
            LP.MenuItem {
                text: qsTr("SearchConfig")
                onTriggered: {
                    Applet.toggleGrandSearchConfig()
                }
            }
        }
    }
}
