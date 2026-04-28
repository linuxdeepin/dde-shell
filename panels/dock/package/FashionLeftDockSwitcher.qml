// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

Control {
    id: root

    required property var model

    property int currentIndex: 0

    readonly property int itemCount: model ? model.count : 0
    readonly property bool hasMultipleItems: itemCount > 1
    readonly property var currentDelegate: switcherRepeater.itemAt(currentIndex)
    readonly property int switchButtonSize: Math.max(18, Math.round(Panel.rootObject.dockItemMaxSize * 0.52))
    readonly property color switchButtonTextColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.9) : Qt.rgba(0, 0, 0, 0.85)
    readonly property color switchButtonHoverColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.08)
    readonly property color switchButtonPressedColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.24) : Qt.rgba(0, 0, 0, 0.14)
    readonly property color switchButtonBorderColor: Panel.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.08)

    visible: itemCount > 0
    implicitWidth: rowLayout.implicitWidth
    implicitHeight: currentDelegate ? currentDelegate.implicitHeight : Panel.rootObject.dockSize

    function switchBy(delta) {
        if (!hasMultipleItems) {
            return
        }

        const nextIndex = (currentIndex + delta + itemCount) % itemCount
        currentIndex = nextIndex
    }

    onItemCountChanged: {
        if (itemCount === 0) {
            currentIndex = 0
        } else if (currentIndex >= itemCount) {
            currentIndex = itemCount - 1
        }
    }

    background: null

    component SwitchButton: Item {
        id: buttonRoot

        required property string glyph
        property bool hovered: buttonArea.containsMouse
        property bool pressed: buttonArea.pressed

        signal clicked()

        implicitWidth: root.switchButtonSize
        implicitHeight: root.switchButtonSize
        opacity: enabled ? 1 : 0.45

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: !buttonRoot.enabled ? "transparent" : (buttonRoot.pressed ? root.switchButtonPressedColor :
                (buttonRoot.hovered ? root.switchButtonHoverColor : "transparent"))
            border.width: (buttonRoot.hovered || buttonRoot.pressed) ? 1 : 0
            border.color: root.switchButtonBorderColor
            antialiasing: true
        }

        Text {
            anchors.centerIn: parent
            text: buttonRoot.glyph
            color: root.switchButtonTextColor
            font.pixelSize: Math.max(14, Math.round(parent.height * 0.6))
            font.weight: Font.Normal
            renderType: Text.NativeRendering
        }

        MouseArea {
            id: buttonArea
            anchors.fill: parent
            enabled: buttonRoot.enabled
            hoverEnabled: true
            onClicked: buttonRoot.clicked()
        }
    }

    contentItem: RowLayout {
        id: rowLayout
        spacing: Math.max(4, Math.round(Panel.rootObject.dockItemMaxSize * 0.08))

        SwitchButton {
            id: previousButton
            visible: root.hasMultipleItems
            enabled: root.hasMultipleItems
            glyph: "\u2039"
            onClicked: root.switchBy(-1)
        }

        Item {
            id: viewport
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: root.currentDelegate ? root.currentDelegate.implicitWidth : 0
            implicitHeight: root.currentDelegate ? root.currentDelegate.implicitHeight : Panel.rootObject.dockSize

            Repeater {
                id: switcherRepeater
                model: root.model

                delegate: Item {
                    id: delegateRoot

                    required property int index

                    property var appletItem: model.data
                    property var attachedAppletItem: null

                    visible: index === root.currentIndex
                    width: implicitWidth
                    height: implicitHeight
                    implicitWidth: appletItem ? appletItem.implicitWidth : 0
                    implicitHeight: appletItem ? appletItem.implicitHeight : Panel.rootObject.dockSize

                    function attachAppletItem() {
                        if (attachedAppletItem && attachedAppletItem !== appletItem && attachedAppletItem.parent === delegateRoot) {
                            attachedAppletItem.parent = null
                        }

                        if (appletItem) {
                            appletItem.parent = delegateRoot
                            attachedAppletItem = appletItem
                        } else {
                            attachedAppletItem = null
                        }
                    }

                    onAppletItemChanged: attachAppletItem()

                    Component.onCompleted: {
                        attachAppletItem()
                    }

                    Component.onDestruction: {
                        if (attachedAppletItem && attachedAppletItem.parent === delegateRoot) {
                            attachedAppletItem.parent = null
                        }
                        attachedAppletItem = null
                    }
                }
            }
        }

        SwitchButton {
            id: nextButton
            visible: root.hasMultipleItems
            enabled: root.hasMultipleItems
            glyph: "\u203A"
            onClicked: root.switchBy(1)
        }
    }
}
