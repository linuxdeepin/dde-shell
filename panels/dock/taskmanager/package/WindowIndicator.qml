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
    required property AppItemPalette palette

    property int borderWidth: (displayMode === Dock.Fashion) ? 1 : 0
    property int radius: 1
    property int dotWidth: 10 + 2 * borderWidth
    property int dotHeight: 2 + 2 * borderWidth

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
            width: dotWidth
            height: dotHeight
            color: palette.dotIndicator
            radius: radius
        }
    }

    Component {
        id: multipleWindows
        Row {
            spacing: 2
            Loader { sourceComponent: dot }
            Loader { sourceComponent: dot }
        }
    }

}
