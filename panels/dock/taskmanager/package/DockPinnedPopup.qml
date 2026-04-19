// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root

    required property var applet
    required property string dockElement
    property var popupWindow: null
    property int colorTheme: Dock.Dark

    signal closeRequested()

    property var descriptor: ({ entries: [] })
    property var pendingDescriptor: null
    property int pendingPopupHeight: 0
    property int popupHeightValue: headerHeight + headerBottomSpacing + 72 + bottomPadding
    property real contentOpacity: 1.0
    property real contentOffsetX: 0.0
    property int lockedColumnCount: 0

    readonly property var entries: descriptor && descriptor.entries ? descriptor.entries : []
    readonly property int sidePadding: 0
    readonly property int headerHeight: 36
    readonly property int headerBottomSpacing: 0
    readonly property int bottomPadding: 8
    readonly property int gridSpacing: 8
    readonly property int itemIconSize: 48
    readonly property int cellWidth: 96
    readonly property int cellHeight: 104
    readonly property int gridWidthExtra: 10
    readonly property int itemTextMaxWidth: 84
    readonly property int itemTopPadding: 4
    readonly property int itemInnerSpacing: 6
    readonly property int itemHoverPadding: 10
    readonly property int itemHoverBottomMargin: 2
    readonly property int itemTextBottomMargin: 6
    readonly property int scrollBarWidth: 6
    readonly property int scrollBarGap: 0
    readonly property int scrollBarLaneWidth: scrollBarWidth
    readonly property int gridWidth: columnCount * cellWidth + Math.max(0, columnCount - 1) * gridSpacing
    readonly property int gridAreaWidth: gridWidth + gridWidthExtra
    readonly property int columnCount: lockedColumnCount > 0 ? lockedColumnCount : preferredColumnCount(descriptor)
    readonly property int totalRows: entries.length === 0 ? 0 : Math.ceil(entries.length / columnCount)
    readonly property int visibleRows: Math.max(1, Math.min(3, totalRows))
    readonly property real wheelOvershootDistance: 28
    readonly property int gridContentHeight: entries.length === 0 ?
                                                72 :
                                                totalRows * cellHeight + Math.max(0, totalRows - 1) * gridSpacing
    readonly property int gridViewportHeight: entries.length === 0 ?
                                                 72 :
                                                 visibleRows * cellHeight + Math.max(0, visibleRows - 1) * gridSpacing
    readonly property int cornerRadius: D.DTK.platformTheme.windowRadius < 0 ? 18 : D.DTK.platformTheme.windowRadius
    readonly property real contentScrollY: contentLoader.item && contentLoader.item.scrollY !== undefined ? contentLoader.item.scrollY : 0
    readonly property bool showHeaderSeparator: contentScrollY > 0.5
    readonly property string middleEllipsis: "\u2026"

    width: sidePadding * 2 + gridAreaWidth + scrollBarLaneWidth
    height: popupHeightValue

    function beginPopupSession() {
        lockedColumnCount = 0
        if (popupWindow) {
            popupWindow.opacity = 1.0
        }
    }

    function preferredColumnCount(nextDescriptor) {
        const nextEntries = nextDescriptor && nextDescriptor.entries ? nextDescriptor.entries.length : 0
        return nextEntries > 12 ? 5 : 4
    }

    function descriptorFor(location) {
        return root.applet ? root.applet.popupDescriptor(root.dockElement, location || "") : ({ entries: [] })
    }

    function popupHeightFor(nextDescriptor) {
        const nextEntries = nextDescriptor && nextDescriptor.entries ? nextDescriptor.entries.length : 0
        const nextRows = nextEntries === 0 ? 0 : Math.ceil(nextEntries / columnCount)
        const nextVisibleRows = Math.max(1, Math.min(3, nextRows))
        const nextGridViewportHeight = nextEntries === 0 ?
                                           72 :
                                           nextVisibleRows * cellHeight + Math.max(0, nextVisibleRows - 1) * gridSpacing
        return headerHeight + headerBottomSpacing + nextGridViewportHeight + bottomPadding
    }

    function commitDescriptor(nextDescriptor) {
        descriptor = nextDescriptor || ({ entries: [] })
        pendingDescriptor = null
        pendingPopupHeight = 0
        popupHeightValue = popupHeightFor(descriptor)
        if (popupWindow) {
            popupWindow.opacity = 1.0
        }
        contentOpacity = 1.0
        contentOffsetX = 0.0
    }

    function trimTextToWidth(metrics, text, maxWidth, fromRight) {
        if (!metrics || !text || maxWidth <= 0) {
            return ({
                text: "",
                width: 0,
                length: 0
            })
        }

        const fullWidth = metrics.advanceWidth(text)
        if (fullWidth <= maxWidth) {
            return ({
                text: text,
                width: fullWidth,
                length: text.length
            })
        }

        let result = ""
        let resultWidth = 0
        if (fromRight) {
            for (let index = text.length - 1; index >= 0; --index) {
                const candidate = text.charAt(index) + result
                const candidateWidth = metrics.advanceWidth(candidate)
                if (candidateWidth > maxWidth) {
                    break
                }
                result = candidate
                resultWidth = candidateWidth
            }
            return ({
                text: result,
                width: resultWidth,
                length: result.length
            })
        }

        for (let index = 0; index < text.length; ++index) {
            const candidate = result + text.charAt(index)
            const candidateWidth = metrics.advanceWidth(candidate)
            if (candidateWidth > maxWidth) {
                break
            }
            result = candidate
            resultWidth = candidateWidth
        }
        return ({
            text: result,
            width: resultWidth,
            length: result.length
        })
    }

    function twoLineElideCandidate(metrics, text, maxWidth, ellipsisOnFirstLine) {
        const ellipsisWidth = metrics.advanceWidth(middleEllipsis)
        const firstLineBudget = Math.max(0, maxWidth - (ellipsisOnFirstLine ? ellipsisWidth : 0))
        const secondLineBudget = Math.max(0, maxWidth - (ellipsisOnFirstLine ? 0 : ellipsisWidth))
        const prefix = trimTextToWidth(metrics, text, firstLineBudget, false)
        const suffix = trimTextToWidth(metrics, text, secondLineBudget, true)
        const hiddenLength = Math.max(0, text.length - prefix.length - suffix.length)

        if (!prefix.length || !suffix.length || hiddenLength <= 0) {
            return null
        }

        const visibleLength = prefix.length + suffix.length
        const balancedLength = Math.min(prefix.length, suffix.length)
        const suffixPenalty = suffix.length <= 1 ? 5000 : (suffix.length === 2 ? 800 : 0)
        const prefixPenalty = prefix.length <= 1 ? 3000 : 0

        return ({
            text: ellipsisOnFirstLine ?
                      prefix.text + middleEllipsis + "\n" + suffix.text :
                      prefix.text + "\n" + middleEllipsis + suffix.text,
            score: visibleLength * 1000 +
                   balancedLength * 2000 +
                   prefix.width +
                   suffix.width -
                   suffixPenalty -
                   prefixPenalty +
                   (ellipsisOnFirstLine ? 100 : 0)
        })
    }

    function twoLineMiddleElidedText(metrics, text, maxWidth, needsElide) {
        if (!text || maxWidth <= 0) {
            return ""
        }

        if (!needsElide) {
            return text
        }

        const candidates = [
            twoLineElideCandidate(metrics, text, maxWidth, true),
            twoLineElideCandidate(metrics, text, maxWidth, false)
        ].filter(function(candidate) { return candidate !== null })

        if (!candidates.length) {
            return trimTextToWidth(metrics, text, maxWidth, false).text
        }

        let bestCandidate = candidates[0]
        for (let index = 1; index < candidates.length; ++index) {
            if (candidates[index].score > bestCandidate.score) {
                bestCandidate = candidates[index]
            }
        }

        return bestCandidate.text
    }

    function clampContentY(flickable, value) {
        if (!flickable) {
            return 0
        }

        return Math.max(0, Math.min(value, Math.max(0, flickable.contentHeight - flickable.height)))
    }

    function overshootContentY(flickable, value) {
        if (!flickable) {
            return 0
        }

        const maxContentY = Math.max(0, flickable.contentHeight - flickable.height)
        if (value < 0) {
            return Math.max(-wheelOvershootDistance, value * 0.35)
        }

        if (value > maxContentY) {
            return maxContentY + Math.min(wheelOvershootDistance, (value - maxContentY) * 0.35)
        }

        return value
    }

    function backIconSource() {
        return Qt.resolvedUrl(root.colorTheme === Dock.Dark ?
                                  "icons/back-chevron-dark.svg" :
                                  "icons/back-chevron-light.svg")
    }

    function refresh(location, animated) {
        const nextDescriptor = descriptorFor(location)
        if (lockedColumnCount <= 0) {
            lockedColumnCount = preferredColumnCount(nextDescriptor)
        }

        if (!animated) {
            commitDescriptor(nextDescriptor)
            return
        }

        if (contentSwapAnimation.running) {
            contentSwapAnimation.stop()
            commitDescriptor(pendingDescriptor || descriptor)
        }

        const currentLocation = descriptor && descriptor.location ? String(descriptor.location) : ""
        const nextLocation = nextDescriptor && nextDescriptor.location ? String(nextDescriptor.location) : ""
        if (currentLocation === nextLocation) {
            commitDescriptor(nextDescriptor)
            return
        }

        pendingDescriptor = nextDescriptor
        pendingPopupHeight = popupHeightFor(nextDescriptor)
        const forward = currentLocation !== "" &&
                        nextDescriptor &&
                        nextDescriptor.parentLocation !== undefined &&
                        String(nextDescriptor.parentLocation) === currentLocation
        contentOffsetX = 0
        contentSwapOutAnimation.to = forward ? -18 : 18
        contentSwapInAnimation.from = forward ? 18 : -18
        contentSwapAnimation.restart()
    }

    onDockElementChanged: refresh("", false)
    Component.onCompleted: refresh("", false)

    Rectangle {
        id: clippedContent
        anchors.fill: parent
        radius: cornerRadius
        color: "transparent"
        clip: true
        Item {
            id: headerRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: headerHeight

            D.ToolButton {
                id: backButton
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.top: parent.top
                anchors.topMargin: 6
                visible: !!(root.descriptor && root.descriptor.canGoBack)
                width: 24
                height: 24
                display: AbstractButton.IconOnly
                flat: true
                padding: 0
                implicitWidth: width
                implicitHeight: height
                contentItem: Item {
                    implicitWidth: 24
                    implicitHeight: 24

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: root.backIconSource()
                        sourceSize: Qt.size(Math.round(width * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)),
                                            Math.round(height * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)))
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                    }
                }
                background: Item {
                    implicitWidth: 24
                    implicitHeight: 24

                    AppletItemBackground {
                        anchors.fill: parent
                        enabled: false
                        radius: 10
                        isActive: false
                        opacity: backButton.hovered || backButton.down ? 1.0 : 0.0
                        Behavior on opacity {
                            NumberAnimation { duration: 150 }
                        }
                    }
                }
                onClicked: root.refresh(root.descriptor.parentLocation, true)
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 72
                text: root.descriptor && root.descriptor.title ? root.descriptor.title : ""
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
                font.weight: Font.Normal
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                height: 1
                color: root.colorTheme === Dock.Dark ?
                           Qt.rgba(1, 1, 1, 0.10) :
                           Qt.rgba(0, 0, 0, 0.10)
                visible: root.showHeaderSeparator
            }
        }

        Item {
            id: listViewport
            width: root.gridAreaWidth + root.scrollBarLaneWidth
            height: root.gridViewportHeight
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: headerRow.bottom
            anchors.topMargin: headerBottomSpacing
            opacity: root.contentOpacity

            transform: Translate {
                x: root.contentOffsetX
            }

            Loader {
                id: contentLoader
                anchors.left: parent.left
                anchors.top: parent.top
                width: root.gridAreaWidth
                height: parent.height
                sourceComponent: root.entries.length === 0 ? emptyStateComponent : gridContentComponent
            }
        }
    }

    Component {
        id: emptyStateComponent

        Item {
            Label {
                anchors.fill: parent
                text: qsTr("No items")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 0.7
            }
        }
    }

    Component {
        id: gridContentComponent

        Item {
            width: root.gridAreaWidth
            height: root.gridViewportHeight
            property real scrollY: gridFlickable.contentY

            Flickable {
                id: gridFlickable
                width: root.gridAreaWidth
                height: root.gridViewportHeight
                anchors.left: parent.left
                anchors.top: parent.top
                property real wheelTargetY: contentY
                contentWidth: root.gridAreaWidth
                contentHeight: root.gridContentHeight
                clip: true
                flickableDirection: Flickable.VerticalFlick
                boundsBehavior: Flickable.DragAndOvershootBounds
                maximumFlickVelocity: 3200
                flickDeceleration: 2800
                interactive: root.totalRows > root.visibleRows
                rebound: Transition {
                    NumberAnimation {
                        properties: "x,y"
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                }

                onDraggingChanged: {
                    if (dragging) {
                        wheelScrollAnimation.stop()
                        wheelTargetY = contentY
                    }
                }
                onContentYChanged: {
                    if (!wheelScrollAnimation.running) {
                        wheelTargetY = contentY
                    }
                }
                onContentHeightChanged: wheelTargetY = root.clampContentY(gridFlickable, wheelTargetY)
                onHeightChanged: wheelTargetY = root.clampContentY(gridFlickable, wheelTargetY)

                WheelHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: function(event) {
                        if (!gridFlickable.interactive) {
                            return
                        }

                        const deltaY = event.pixelDelta.y !== 0 ? event.pixelDelta.y : (event.angleDelta.y / 120) * 40
                        const baseTarget = wheelScrollAnimation.running ? gridFlickable.wheelTargetY : gridFlickable.contentY
                        gridFlickable.wheelTargetY = root.overshootContentY(gridFlickable, baseTarget - deltaY)

                        wheelScrollAnimation.from = gridFlickable.contentY
                        wheelScrollAnimation.to = gridFlickable.wheelTargetY
                        wheelScrollAnimation.restart()
                        event.accepted = true
                    }
                }

                NumberAnimation {
                    id: wheelScrollAnimation
                    target: gridFlickable
                    property: "contentY"
                    duration: 220
                    easing.type: Easing.OutCubic
                    onFinished: {
                        const clampedTarget = root.clampContentY(gridFlickable, gridFlickable.wheelTargetY)
                        if (Math.abs(clampedTarget - gridFlickable.wheelTargetY) > 0.5) {
                            gridFlickable.returnToBounds()
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: listViewport
                    x: listViewport.width - width
                    y: 0
                    height: listViewport.height
                    implicitWidth: root.scrollBarWidth
                    policy: root.totalRows > root.visibleRows ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                Grid {
                    id: contentGrid
                    x: Math.round((root.gridAreaWidth - width) / 2)
                    columns: root.columnCount
                    spacing: root.gridSpacing

                    Repeater {
                        model: root.entries

                        delegate: D.ToolButton {
                            id: gridButton
                            required property var modelData

                            property int hoverWidth: Math.min(root.cellWidth - 4,
                                                              Math.max(root.itemIconSize + root.itemHoverPadding * 2,
                                                                       titleMetrics.advanceWidth + root.itemHoverPadding * 2))
                            property int hoverHeight: Math.min(root.cellHeight - 2,
                                                               contentColumn.implicitHeight + root.itemHoverPadding + root.itemTextBottomMargin - root.itemHoverBottomMargin)

                            width: root.cellWidth
                            height: root.cellHeight
                            flat: true
                            padding: 0

                            background: Item {
                                AppletItemBackground {
                                    enabled: false
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 0
                                    width: gridButton.hoverWidth
                                    height: gridButton.hoverHeight
                                    radius: 12
                                    isActive: false
                                    opacity: gridButton.hovered || gridButton.down ? 1.0 : 0.0
                                    Behavior on opacity {
                                        NumberAnimation { duration: 150 }
                                    }
                                }
                            }

                            contentItem: Item {
                                Column {
                                    id: contentColumn
                                    anchors.top: parent.top
                                    anchors.topMargin: root.itemTopPadding
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: gridButton.hoverWidth - root.itemHoverPadding * 2
                                    spacing: root.itemInnerSpacing

                                    D.DciIcon {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: root.itemIconSize
                                        height: root.itemIconSize
                                        sourceSize: Qt.size(width, height)
                                        name: modelData.iconName
                                        smooth: false
                                        retainWhileLoading: true
                                    }

                                    Label {
                                        id: titleLabel
                                        width: Math.min(root.itemTextMaxWidth, parent.width)
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: root.twoLineMiddleElidedText(titleFontMetrics,
                                                                           titleMeasure.text,
                                                                           width,
                                                                           titleMeasure.lineCount > 2)
                                        wrapMode: Text.WrapAnywhere
                                        maximumLineCount: 2
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideNone
                                        font.pixelSize: 11
                                        font.weight: Font.Normal
                                    }

                                    Text {
                                        id: titleMeasure
                                        width: titleLabel.width
                                        visible: false
                                        text: gridButton.modelData && gridButton.modelData.name ? String(gridButton.modelData.name) : ""
                                        wrapMode: Text.WrapAnywhere
                                        font.pixelSize: titleLabel.font.pixelSize
                                        font.weight: titleLabel.font.weight
                                    }

                                    FontMetrics {
                                        id: titleFontMetrics
                                        font.pixelSize: titleLabel.font.pixelSize
                                        font.weight: titleLabel.font.weight
                                    }

                                    Item {
                                        width: 1
                                        height: root.itemTextBottomMargin
                                    }
                                }
                            }

                            TextMetrics {
                                id: titleMetrics
                                font.pixelSize: 11
                                text: gridButton.modelData && gridButton.modelData.name ? String(gridButton.modelData.name) : ""
                            }

                            onClicked: {
                                if (modelData.directory) {
                                    root.refresh(modelData.entryId, true)
                                    return
                                }

                                root.applet.activatePopupEntry(root.dockElement, modelData.entryId)
                                root.closeRequested()
                            }
                        }
                    }
                }
            }
        }
    }

    SequentialAnimation {
        id: contentSwapAnimation

        ParallelAnimation {
            NumberAnimation {
                target: root.popupWindow
                property: "opacity"
                to: 0.35
                duration: 90
                easing.type: Easing.OutQuad
            }

            NumberAnimation {
                target: root
                property: "contentOpacity"
                to: 0
                duration: 90
                easing.type: Easing.OutQuad
            }

            NumberAnimation {
                id: contentSwapOutAnimation
                target: root
                property: "contentOffsetX"
                to: -18
                duration: 90
                easing.type: Easing.OutQuad
            }
        }

        ScriptAction {
            script: {
                root.descriptor = root.pendingDescriptor || ({ entries: [] })
                root.popupHeightValue = root.pendingPopupHeight > 0 ? root.pendingPopupHeight : root.popupHeightFor(root.descriptor)
                root.contentOpacity = 0
                root.contentOffsetX = contentSwapInAnimation.from
            }
        }

        ParallelAnimation {
            NumberAnimation {
                target: root.popupWindow
                property: "opacity"
                to: 1.0
                duration: 150
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: root
                property: "contentOpacity"
                to: 1
                duration: 150
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                id: contentSwapInAnimation
                target: root
                property: "contentOffsetX"
                from: 18
                to: 0
                duration: 150
                easing.type: Easing.OutCubic
            }
        }

        ScriptAction {
            script: {
                root.pendingDescriptor = null
                root.pendingPopupHeight = 0
            }
        }
    }
}
