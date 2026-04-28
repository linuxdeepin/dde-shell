// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root

    property string iconName: ""
    property var previewIcons: []
    property int iconSize: 32
    property int colorTheme: Dock.Dark
    readonly property var visiblePreviewIcons: {
        const icons = []
        if (!previewIcons) {
            return icons
        }

        for (let i = 0; i < previewIcons.length && icons.length < 4; ++i) {
            const icon = previewIcons[i]
            if (icon && icon.length > 0) {
                icons.push(icon)
            }
        }
        return icons
    }
    readonly property bool useCompositePreview: visiblePreviewIcons.length > 1
    readonly property int desiredCompositeOuterInset: Math.max(2, Math.round(iconSize * 0.12))
    readonly property int desiredCompositeGap: Math.max(1, Math.round(iconSize * 0.05))
    readonly property int compositeIconSize: Math.max(1, Math.floor((iconSize - desiredCompositeOuterInset * 2 - desiredCompositeGap) / 2))
    readonly property int compositeGap: Math.max(1, Math.min(desiredCompositeGap, Math.max(1, iconSize - compositeIconSize * 2)))
    readonly property int compositeContentSize: compositeIconSize * 2 + compositeGap
    readonly property int compositeOuterInset: Math.max(1, Math.floor((iconSize - compositeContentSize) / 2))

    width: iconSize
    height: iconSize

    Rectangle {
        anchors.fill: parent
        radius: Math.max(6, Math.round(root.iconSize / 4))
        color: root.colorTheme === Dock.Dark ?
                   Qt.rgba(1, 1, 1, 0.10) :
                   Qt.rgba(0, 0, 0, 0.10)
        border.width: 1
        border.color: root.colorTheme === Dock.Dark ?
                          Qt.rgba(1, 1, 1, 0.40) :
                          Qt.rgba(0, 0, 0, 0.20)
        visible: root.useCompositePreview
    }

    Item {
        anchors.centerIn: parent
        width: root.compositeContentSize
        height: root.compositeContentSize
        visible: root.useCompositePreview

        Grid {
            anchors.centerIn: parent
            columns: 2
            rows: 2
            rowSpacing: root.compositeGap
            columnSpacing: root.compositeGap

            Repeater {
                model: root.useCompositePreview ? root.visiblePreviewIcons : 0
                delegate: D.DciIcon {
                    required property string modelData

                    name: modelData
                    width: root.compositeIconSize
                    height: root.compositeIconSize
                    sourceSize: Qt.size(width, height)
                    smooth: false
                    retainWhileLoading: true
                }
            }
        }
    }

    D.DciIcon {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        sourceSize: Qt.size(width, height)
        name: root.visiblePreviewIcons.length === 1 ? root.visiblePreviewIcons[0] : root.iconName
        visible: !root.useCompositePreview
        smooth: false
        retainWhileLoading: true
    }
}
