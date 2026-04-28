// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import org.deepin.ds.dock 1.0

Item {
    id: root
    required property list<string> windows
    required property int displayMode
    required property bool useColumnLayout
    required property bool compactFashionIndicator
    required property AppItemPalette palette
    required property int dotWidth
    required property int dotHeight
    required property int multiDotWidth
    required property int multiDotHeight

    property real borderWidth: compactFashionIndicator ? 1 : ((displayMode === Dock.Fashion) ? 1 : 0)
    readonly property real compactFashionOuterRadius: 2
    readonly property real compactFashionInnerRadius: Math.max(0, compactFashionOuterRadius - borderWidth)
    readonly property real singleIndicatorWidth: compactFashionIndicator
                                              ? dotWidth + 2 * borderWidth
                                              : ((root.displayMode === Dock.Fashion || useColumnLayout) ? dotWidth : dotWidth / 2 - 1) + 2 * borderWidth
    readonly property real singleIndicatorHeight: compactFashionIndicator
                                               ? dotHeight + 2 * borderWidth
                                               : ((root.displayMode === Dock.Fashion || !useColumnLayout) ? dotHeight : dotHeight / 2 - 1) + 2 * borderWidth
    readonly property real multiIndicatorWidth: compactFashionIndicator ? multiDotWidth + 2 * borderWidth : singleIndicatorWidth
    readonly property real multiIndicatorHeight: compactFashionIndicator ? multiDotHeight + 2 * borderWidth : singleIndicatorHeight

    width: indicatorLoader.width
    height: indicatorLoader.height

    Loader {
        id: indicatorLoader
        anchors.centerIn: parent
        sourceComponent: (windows.length > 1) ? multipleWindows : dot
    }

    Component {
        id: dot
        Item {
            width: root.singleIndicatorWidth
            height: root.singleIndicatorHeight

            Rectangle {
                anchors.fill: parent
                color: palette.dotIndicatorBorder
                antialiasing: compactFashionIndicator
                radius: compactFashionIndicator ? compactFashionOuterRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: borderWidth
                color: palette.dotIndicator
                antialiasing: compactFashionIndicator
                radius: compactFashionIndicator ? compactFashionInnerRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
            }
        }
    }

    Component {
        id: multipleWindows
        Grid {
            columns: useColumnLayout ?  1 : 2
            rows: useColumnLayout ? 2 : 1
            flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
            spacing: 2
            Item {
                width: root.multiIndicatorWidth
                height: root.multiIndicatorHeight

                Rectangle {
                    anchors.fill: parent
                    color: palette.dotIndicatorBorder
                    antialiasing: compactFashionIndicator
                    radius: compactFashionIndicator ? compactFashionOuterRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: borderWidth
                    color: palette.dotIndicator
                    antialiasing: compactFashionIndicator
                    radius: compactFashionIndicator ? compactFashionInnerRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
                }
            }
            Item {
                width: root.multiIndicatorWidth
                height: root.multiIndicatorHeight

                Rectangle {
                    anchors.fill: parent
                    color: palette.dotIndicatorBorder
                    antialiasing: compactFashionIndicator
                    radius: compactFashionIndicator ? compactFashionOuterRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: borderWidth
                    color: palette.dotIndicator
                    antialiasing: compactFashionIndicator
                    radius: compactFashionIndicator ? compactFashionInnerRadius : ((root.displayMode === Dock.Fashion || useColumnLayout) ? width / 2 : height / 2)
                }
            }
        }
    }

}
