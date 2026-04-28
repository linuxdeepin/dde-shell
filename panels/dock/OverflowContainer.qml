// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 2.15

Item {
    id: root
    required property bool useColumnLayout
    property alias model: listView.model
    property alias delegate: listView.delegate
    property alias spacing: listView.spacing
    property alias count: listView.count
    property alias add: listView.add
    property alias remove: listView.remove
    property alias move: listView.move
    property alias displaced: listView.displaced
    property alias addDisplaced: listView.addDisplaced
    property alias removeDisplaced: listView.removeDisplaced
    property alias moveDisplaced: listView.moveDisplaced
    property alias footer: listView.footer
    property alias footerPositioning: listView.footerPositioning
    ListView {
        id: listView
        anchors.fill: parent
        orientation: useColumnLayout ? ListView.Vertical : ListView.Horizontal
        layoutDirection: Qt.LeftToRight
        verticalLayoutDirection: ListView.TopToBottom
        interactive: false
    }

    function calculateImplicitWidth(prev, current) {
        if (useColumnLayout) {
            return Math.max(prev, current)
        } else {
            if (prev == 0) {
                return current
            }
            return prev + root.spacing + current
        }
    }

    function calculateImplicitHeight(prev, current) {
        if (useColumnLayout) {
            if (prev == 0) {
                return current
            }
            return prev + root.spacing + current
        } else {
            return Math.max(prev, current)
        }
    }

    function indexAt(x, y) {
        return listView.indexAt(x, y)
    }

    function childTargetImplicitWidth(child) {
        if (!child) {
            return 0
        }

        if (child.targetImplicitWidth !== undefined) {
            return child.targetImplicitWidth
        }

        return child.implicitWidth
    }

    function childTargetImplicitHeight(child) {
        if (!child) {
            return 0
        }

        if (child.targetImplicitHeight !== undefined) {
            return child.targetImplicitHeight
        }

        return child.implicitHeight
    }

    implicitWidth: {
        let width = 0
        for (let child of listView.contentItem.visibleChildren) {
            width = calculateImplicitWidth(width, child.implicitWidth)
        }
        // TODO: abvoe qt6.8 implicitSize to 0 will make size to 0 default.
        // so make minimum implicitSize to 1, find why and remove below
        return Math.max(width, 1)
    }
    implicitHeight: {
        let height = 0
        for (let child of listView.contentItem.visibleChildren) {
            height = calculateImplicitHeight(height, child.implicitHeight)
        }
        return Math.max(height, 1)
    }

    readonly property real targetImplicitWidth: {
        let width = 0
        for (let child of listView.contentItem.visibleChildren) {
            width = calculateImplicitWidth(width, childTargetImplicitWidth(child))
        }
        return Math.max(width, 1)
    }

    readonly property real targetImplicitHeight: {
        let height = 0
        for (let child of listView.contentItem.visibleChildren) {
            height = calculateImplicitHeight(height, childTargetImplicitHeight(child))
        }
        return Math.max(height, 1)
    }
}
