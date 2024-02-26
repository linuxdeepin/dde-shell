// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 2.15

Item {
    id: root
    // NOTE: if the value is assigned, and not -1, it will use this value as implicitWidth
    property int assignedWidth: -1
    property int assignedHeight: -1
    required property bool useColumnLayout
    property alias model: listView.model
    property alias delegate: listView.delegate
    property alias spacing: listView.spacing
    property alias count: listView.count
    property alias displaced: listView.displaced

    // NOTE: the width calculated by contentItems
    property int suggestedWidth: {
        let width = 0
        for (let child of listView.contentItem.visibleChildren) {
            width = calculateImplicitWidth(width, child.implicitWidth)
        }
        return width
    }

    // NOTE: the height calculated by contentItems
    property int suggestedHeight: {
        let height = 0
        for (let child of listView.contentItem.visibleChildren) {
            height = calculateImplicitHeight(height, child.implicitHeight)
        }
        return height
    }

    // NOTE: provide width value for other objects
    property int suggestedImplicitWidth : {
        if (root.assignedWidth !== -1) {
            return root.assignedWidth
        }
        return root.suggestedWidth
    }

    // NOTE: provide height value for other objects
    property int suggestedImplicitHeight : {
        if (root.assignedHeight !== -1) {
            return root.assignedHeight
        }
        return root.suggestedHeight
    }

    ListView {
        id: listView
        anchors.fill: parent
        orientation: useColumnLayout ? ListView.Vertical : ListView.Horizontal
        layoutDirection: Qt.LeftToRight
        verticalLayoutDirection: ListView.TopToBottom
        interactive: false
        clip: true

    }

    function calculateImplicitWidth(prev, current) {
        if (useColumnLayout) {
            return Math.max(prev, current)
        } else {
            return prev + current + root.spacing
        }
    }

    function calculateImplicitHeight(prev, current) {
        if (useColumnLayout) {
            return prev + current + root.spacing
        } else {
            return Math.max(prev, current)
        }
    }

    implicitWidth: root.suggestedImplicitWidth

    implicitHeight: root.suggestedImplicitHeight
}
