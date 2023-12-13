// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: appItem
    required property int displayMode
    required property int colorTheme
    required property bool active
    required property string itemId
    required property string name
    required property string iconName
    required property string menus
    required property list<string> windows

    signal clickItem(itemId: string)
    signal clickItemMenu(itemId: string, menuId: string)

    width: 40
    height: 40

    D.DciIcon {
        id: icon
        name: iconName
        sourceSize: Qt.size(24, 24)
        anchors.centerIn: parent
    }

    AppItemPalette {
        id: itemPalette
        displayMode: appItem.displayMode
        colorTheme: appItem.colorTheme
        active: appItem.active
    }

    Loader {
        id: statusIndicatorLoader
        anchors.centerIn: parent
        property int displayMode: appItem.displayMode
        sourceComponent: (displayMode === Dock.Efficient && windows.length > 0) ? statusIndicator : null
    }

    Loader {
        id: windowIndicatorLoader
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 1
        }
        property int displayMode: appItem.displayMode
        property list<string> windows: appItem.windows
        sourceComponent: (displayMode === Dock.Efficient && windows.length > 1) || (displayMode === Dock.Fashion && windows.length > 0) ? windowIndicator : null
    }

    Component {
        id: statusIndicator
        StatusIndicator {
            palette: itemPalette
        }
    }

    Component {
        id: windowIndicator
        WindowIndicator {
            windows: appItem.windows
            displayMode: appItem.displayMode
            palette: itemPalette
        }
    }

    MouseArea {
        id: clickArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup()
            else
                appItem.clickItem(itemId)
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
                    appItem.clickItemMenu(itemId, modelData.id)
                }
            }
        }
    }
}
