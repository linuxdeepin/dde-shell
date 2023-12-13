// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 2.15

Item {
    id: container
    width: gridLayout.width
    height: gridLayout.height
    enum Direction {
        LeftToRight,
        TopToBottom
    }
    required property int direction
    default property alias content: gridLayout.children
    GridLayout {
        id: gridLayout
        anchors.centerIn: parent
        columns: 1
        rows: 1
        flow: container.direction === OverflowContainer.Direction.LeftToRight ? GridLayout.TopToBottom : GridLayout.LeftToRight
    }
}
