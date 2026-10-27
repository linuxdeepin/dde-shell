// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import org.deepin.dtk 1.0 as D

Rectangle {
    id: root
    property var presentation: null
    property var contents: []
    readonly property bool providerContent: presentation !== null && presentation !== undefined
    readonly property bool empty: contents.length === 0
    radius: providerContent ? 0 : height / 6
    color: providerContent || empty ? "transparent" : Qt.rgba(1, 1, 1, 0.2)

    onPresentationChanged: {
        const url = root.presentation ? root.presentation.iconUrl : ""
        providerIconLoader.setSource(url, url ? {presentation: root.presentation} : {})
    }

    Loader {
        id: providerIconLoader
        objectName: "providerIconLoader"
        anchors.fill: parent
    }

    D.DciIcon {
        objectName: "folderPlaceholder"
        anchors.fill: parent
        sourceSize: Qt.size(width, height)
        name: "folder"
        visible: root.providerContent ? providerIconLoader.status !== Loader.Ready : root.empty
    }

    Item {
        anchors.fill: parent
        visible: !root.providerContent

        // The translucent plate only frames the preview grid; an empty folder
        // just shows the folder glyph on the bare dock background.
        D.BoxShadow {
            anchors.fill: parent
            z: -1
            shadowColor: Qt.rgba(0, 0, 0, 0.1)
            shadowBlur: 10
            shadowOffsetX: 0
            shadowOffsetY: 0
            cornerRadius: root.radius
            spread: 0
            hollow: true
            visible: root.contents.length !== 0
        }

        Item {
            anchors.fill: parent
            anchors.margins: root.width / 8
            Repeater {
                model: 4
                D.DciIcon {
                    required property int index
                    objectName: "folderPreview" + index
                    width: (root.width * 3 / 4 - root.width / 12) / 2
                    height: width
                    name: index < root.contents.length ? root.contents[index].iconName : ""
                    visible: index < root.contents.length
                    sourceSize: Qt.size(width, height)
                    // Invisible slots still participate in the fixed 2x2 geometry.
                    x: (index % 2) * (width + root.width / 12)
                    y: Math.floor(index / 2) * (height + root.width / 12)
                }
            }
        }
    }
}
