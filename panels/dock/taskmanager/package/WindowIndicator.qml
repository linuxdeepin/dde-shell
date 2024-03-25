// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import org.deepin.ds.dock 1.0

Item {
    required property list<string> windows
    required property int displayMode
    required property bool useColumnLayout
    required property AppItemPalette palette
    required property int dotWidth
    required property int dotHeight

    property int borderWidth: (displayMode === Dock.Fashion) ? 1 : 0
    property int radius: 1

    width: indicatorLoader.width
    height: indicatorLoader.height

    Loader {
        id: indicatorLoader
        anchors.centerIn: parent
        sourceComponent: (windows.length > 1) ? multipleWindows : dot
    }

    Component {
        id: dot
        Rectangle {
            border.width: borderWidth
            border.color: palette.dotIndicatorBorder
            width: dotWidth + 2 * borderWidth
            height: dotHeight + 2 * borderWidth
            color: palette.dotIndicator
            radius: radius
        }
    }

    Component {
        id: multipleWindows
        Grid {
            columns: useColumnLayout ?  1 : 2
            rows: useColumnLayout ? 2 : 1
            flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
            spacing: 2
            Loader { sourceComponent: dot }
            Loader { sourceComponent: dot }
        }
    }

}
