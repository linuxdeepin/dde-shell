// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    required property bool useColumnLayout
    property bool atViewBeginning: useColumnLayout ? listView.atYBeginning : listView.atXBeginning
    property bool atViewEnd: useColumnLayout ? listView.atYEnd : listView.atXEnd
    property alias model: listView.model
    property alias delegate: listView.delegate
    property alias spacing: listView.spacing
    property alias count: listView.count
    property alias add: listView.add
    property alias remove: listView.remove
    property alias move: listView.move
    property alias displaced: listView.displaced
    property alias interactive: listView.interactive
    property alias header: listView.header
    property alias footer: listView.footer

    function forceLayout() {
        listView.forceLayout()
    }

    function scrollIncrease() {
        if (useColumnLayout) {
            vsb.increase()
        } else {
            hsb.increase()
        }
    }

    function scrollDecrease() {
        if (useColumnLayout) {
            vsb.decrease()
        } else {
            hsb.decrease()
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        orientation: root.useColumnLayout ? ListView.Vertical : ListView.Horizontal
        layoutDirection: Qt.LeftToRight
        verticalLayoutDirection: ListView.TopToBottom
        boundsBehavior: Flickable.StopAtBounds
        interactive: false
        cacheBuffer: 100
        ScrollBar.horizontal: ScrollBar {
            id: hsb
            enabled: !root.useColumnLayout
            policy: ScrollBar.AlwaysOff
        }
        ScrollBar.vertical: ScrollBar {
            id: vsb
            enabled: root.useColumnLayout
            policy: ScrollBar.AlwaysOff
        }
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

    implicitWidth: {
        let width = 0
        for (let child of listView.contentItem.visibleChildren) {
            width = calculateImplicitWidth(width, child.implicitWidth)
        }
        // TODO: above qt6.8 implicitSize to 0 will make size to 0 default.
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
}
