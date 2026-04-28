// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

FocusScope {
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
    property string typeAheadBuffer: ""
    property int keyboardCurrentIndex: -1
    property bool keyboardSelectionActive: false

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
    readonly property int scrollBarWidth: 0
    readonly property int scrollBarGap: 0
    readonly property int scrollBarLaneWidth: scrollBarWidth
    readonly property int thumbnailCornerRadius: 3
    readonly property color selectionFillColor: Qt.rgba(1, 1, 1, 0.21)
    readonly property color selectionInsideBorderColor: Qt.rgba(1, 1, 1, 0.15)
    readonly property color selectionOutsideBorderColor: Qt.rgba(0, 0, 0, 0.14)
    readonly property color hoverFillColor: Qt.rgba(1, 1, 1, 0.08)
    readonly property color hoverInsideBorderColor: Qt.rgba(1, 1, 1, 0.06)
    readonly property color hoverOutsideBorderColor: Qt.rgba(0, 0, 0, 0.06)
    readonly property color thumbnailInsideBorderColor: root.colorTheme === Dock.Dark ?
                                                         Qt.rgba(1, 1, 1, 0.14) :
                                                         Qt.rgba(1, 1, 1, 0.45)
    readonly property color thumbnailOutsideBorderColor: root.colorTheme === Dock.Dark ?
                                                          Qt.rgba(0, 0, 0, 0.24) :
                                                          Qt.rgba(0, 0, 0, 0.12)
    readonly property int gridWidth: columnCount * cellWidth + Math.max(0, columnCount - 1) * gridSpacing
    readonly property int gridAreaWidth: gridWidth + gridWidthExtra
    readonly property int columnCount: lockedColumnCount > 0 ? lockedColumnCount : preferredColumnCount(descriptor)
    readonly property int totalRows: entries.length === 0 ? 0 : Math.ceil(entries.length / columnCount)
    readonly property int visibleRows: Math.max(1, Math.min(3, totalRows))
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
    readonly property bool popupOwnerActive: !!(root.parent && root.parent.visible)

    width: sidePadding * 2 + gridAreaWidth + scrollBarLaneWidth
    height: popupHeightValue
    focus: true

    function beginPopupSession() {
        clearTypeAhead()
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
        resetKeyboardNavigation()
        if (popupWindow) {
            popupWindow.opacity = 1.0
        }
        contentOpacity = 1.0
        contentOffsetX = 0.0
        ensureKeyboardFocus()
    }

    function clearDescriptor() {
        descriptor = ({ entries: [] })
        pendingDescriptor = null
        pendingPopupHeight = 0
        popupHeightValue = popupHeightFor(descriptor)
        contentOpacity = 1.0
        contentOffsetX = 0.0
        resetKeyboardNavigation()
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

    function parentDirectoryFor(path) {
        if (!path) {
            return ""
        }

        const normalizedPath = String(path)
        const separatorIndex = normalizedPath.lastIndexOf("/")
        return separatorIndex > 0 ? normalizedPath.substring(0, separatorIndex) : ""
    }

    function backIconSource() {
        return Qt.resolvedUrl(root.colorTheme === Dock.Dark ?
                                  "icons/back-chevron-dark.svg" :
                                  "icons/back-chevron-light.svg")
    }

    function currentLocation() {
        return descriptor && descriptor.location ? String(descriptor.location) : ""
    }

    function clearTypeAhead() {
        typeAheadResetTimer.stop()
        typeAheadBuffer = ""
    }

    function clearKeyboardSelection() {
        keyboardCurrentIndex = -1
        keyboardSelectionActive = false
    }

    function resetKeyboardNavigation() {
        clearTypeAhead()
        clearKeyboardSelection()
    }

    function ensureKeyboardFocus() {
        if (!root.popupOwnerActive || !root.popupWindow || !root.popupWindow.visible) {
            return
        }

        focusRestoreTimer.restart()
    }

    function normalizedEntryName(value) {
        return String(value || "").toLocaleLowerCase()
    }

    function typeAheadTextForKey(key, text) {
        const keyText = String(text || "")
        if (keyText.length === 1 && keyText.charCodeAt(0) >= 0x20) {
            return keyText
        }

        if (key >= Qt.Key_A && key <= Qt.Key_Z) {
            return String.fromCharCode("a".charCodeAt(0) + key - Qt.Key_A)
        }

        if (key >= Qt.Key_0 && key <= Qt.Key_9) {
            return String.fromCharCode("0".charCodeAt(0) + key - Qt.Key_0)
        }

        return ""
    }

    function findPrefixMatch(prefix) {
        if (!prefix) {
            return -1
        }

        const normalizedPrefix = normalizedEntryName(prefix)
        for (let index = 0; index < entries.length; ++index) {
            const entryName = entries[index] && entries[index].name !== undefined ?
                                  normalizedEntryName(entries[index].name) :
                                  ""
            if (entryName.startsWith(normalizedPrefix)) {
                return index
            }
        }

        return -1
    }

    function selectEntryIndex(index, animated) {
        if (index < 0 || index >= entries.length) {
            clearKeyboardSelection()
            return false
        }

        keyboardCurrentIndex = index
        keyboardSelectionActive = true
        if (contentLoader.item && contentLoader.item.scrollToEntry) {
            contentLoader.item.scrollToEntry(index, animated !== false)
        }
        return true
    }

    function moveKeyboardSelection(delta) {
        if (!entries.length) {
            clearKeyboardSelection()
            return false
        }

        if (!keyboardSelectionActive || keyboardCurrentIndex < 0 || keyboardCurrentIndex >= entries.length) {
            return selectEntryIndex(0, false)
        }

        const currentIndex = keyboardCurrentIndex
        const nextIndex = Math.max(0, Math.min(entries.length - 1, currentIndex + delta))
        return selectEntryIndex(nextIndex, true)
    }

    function moveKeyboardSelectionTo(index) {
        if (!entries.length) {
            clearKeyboardSelection()
            return false
        }

        const nextIndex = Math.max(0, Math.min(entries.length - 1, index))
        return selectEntryIndex(nextIndex, true)
    }

    function handleTypeAheadInput(text) {
        const addition = String(text || "")
        if (!addition.length) {
            return
        }

        let nextBuffer = typeAheadBuffer + addition
        let matchIndex = findPrefixMatch(nextBuffer)
        if (matchIndex < 0) {
            nextBuffer = addition
            matchIndex = findPrefixMatch(nextBuffer)
            if (matchIndex < 0) {
                typeAheadResetTimer.restart()
                return
            }
        }

        typeAheadBuffer = nextBuffer
        typeAheadResetTimer.restart()
        selectEntryIndex(matchIndex, true)
    }

    function handleTypeAheadBackspace() {
        if (!typeAheadBuffer.length) {
            clearTypeAhead()
            return
        }

        typeAheadBuffer = typeAheadBuffer.slice(0, -1)
        if (!typeAheadBuffer.length) {
            clearTypeAhead()
            return
        }

        typeAheadResetTimer.restart()
        selectEntryIndex(findPrefixMatch(typeAheadBuffer), true)
    }

    function handlePopupKeyPress(key, text, modifiers) {
        if (!root.popupOwnerActive || !root.popupWindow || !root.popupWindow.visible) {
            return false
        }

        if (key === Qt.Key_Escape) {
            if (root.typeAheadBuffer.length || root.keyboardSelectionActive) {
                root.resetKeyboardNavigation()
                return true
            }
            return false
        }

        if (key === Qt.Key_Backspace) {
            root.handleTypeAheadBackspace()
            return true
        }

        if (key === Qt.Key_Left) {
            return root.moveKeyboardSelection(-1)
        }

        if (key === Qt.Key_Right) {
            return root.moveKeyboardSelection(1)
        }

        if (key === Qt.Key_Up) {
            return root.moveKeyboardSelection(-root.columnCount)
        }

        if (key === Qt.Key_Down) {
            return root.moveKeyboardSelection(root.columnCount)
        }

        if (key === Qt.Key_Home) {
            return root.moveKeyboardSelectionTo(0)
        }

        if (key === Qt.Key_End) {
            return root.moveKeyboardSelectionTo(root.entries.length - 1)
        }

        if (key === Qt.Key_Return || key === Qt.Key_Enter) {
            if (root.keyboardSelectionActive) {
                root.activateKeyboardSelection()
                return true
            }
            return false
        }

        if ((modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)) !== 0) {
            return false
        }

        const keyText = root.typeAheadTextForKey(key, text)
        if (!keyText.length) {
            return false
        }

        root.handleTypeAheadInput(keyText)
        return true
    }

    function handleApplicationKeyEvent(key, text, modifiers) {
        return handlePopupKeyPress(Number(key), String(text || ""), Number(modifiers))
    }

    function activateKeyboardSelection() {
        if (!keyboardSelectionActive || keyboardCurrentIndex < 0 || keyboardCurrentIndex >= entries.length) {
            return
        }

        const currentEntry = entries[keyboardCurrentIndex]
        if (!currentEntry) {
            return
        }

        if (currentEntry.directory) {
            refresh(currentEntry.entryId, true)
            return
        }

        root.applet.activatePopupEntry(root.dockElement, currentEntry.entryId)
        root.closeRequested()
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

    onDockElementChanged: {
        if (root.popupWindow && root.popupWindow.visible) {
            refresh("", false)
            return
        }

        clearDescriptor()
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: function(event) {
        event.accepted = root.handlePopupKeyPress(event.key, event.text, event.modifiers)
    }

    onActiveFocusChanged: {
        if (root.popupOwnerActive && !root.activeFocus && root.popupWindow && root.popupWindow.visible && !root.popupWindow.active) {
            root.closeRequested()
        }
    }

    Connections {
        target: root.applet

        function onPopupEntryThumbnailChanged(entryPath) {
            if (!entryPath ||
                !root.descriptor ||
                root.descriptor.kind !== "folder" ||
                !root.descriptor.location ||
                (root.popupWindow && !root.popupWindow.visible)) {
                return
            }

            if (root.parentDirectoryFor(entryPath) === String(root.descriptor.location)) {
                thumbnailRefreshTimer.restart()
            }
        }
    }

    Connections {
        target: root.popupWindow
        enabled: root.popupOwnerActive

        function onVisibleChanged() {
            if (!root.popupWindow) {
                return
            }

            if (root.popupWindow.visible) {
                root.ensureKeyboardFocus()
            } else {
                root.resetKeyboardNavigation()
            }
        }

        function onActiveChanged() {
            if (!root.popupWindow) {
                return
            }

            if (root.popupWindow.active) {
                root.ensureKeyboardFocus()
            } else if (root.popupWindow.visible) {
                root.closeRequested()
            }
        }
    }

    Timer {
        id: thumbnailRefreshTimer
        interval: 60
        repeat: false
        onTriggered: {
            if (root.descriptor &&
                root.descriptor.kind === "folder" &&
                root.descriptor.location &&
                (!root.popupWindow || root.popupWindow.visible)) {
                root.refresh(String(root.descriptor.location), false)
            }
        }
    }

    Timer {
        id: typeAheadResetTimer
        interval: 900
        repeat: false
        onTriggered: root.clearTypeAhead()
    }

    Timer {
        id: focusRestoreTimer
        interval: 1
        repeat: false
        onTriggered: {
            if (root.popupOwnerActive && root.popupWindow && root.popupWindow.visible) {
                root.forceActiveFocus(Qt.OtherFocusReason)
            }
        }
    }

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
            id: gridContentRoot
            width: root.gridAreaWidth
            height: root.gridViewportHeight
            property real scrollY: gridFlickable.contentY
            property Item hoverTargetButton: null
            property Item selectionTargetButton: null
            property bool hoverSyncPending: false
            property real hoverBackgroundTargetX: 0
            property real hoverBackgroundTargetY: 0
            property bool hoverBackgroundPositionAnimationEnabled: false

            function hoverBackgroundXFor(button) {
                if (!button) {
                    return hoverBackgroundTargetX
                }

                return contentGrid.x + button.x + Math.round((button.width - button.hoverWidth) / 2)
            }

            function hoverBackgroundYFor(button) {
                if (!button) {
                    return hoverBackgroundTargetY
                }

                return contentGrid.y + button.y
            }

            function setHoverTarget(nextTarget, animatePosition) {
                const nextTargetX = nextTarget ? hoverBackgroundXFor(nextTarget) : hoverBackgroundTargetX
                const nextTargetY = nextTarget ? hoverBackgroundYFor(nextTarget) : hoverBackgroundTargetY
                const targetChanged = hoverTargetButton !== nextTarget
                const positionChanged = !nextTarget
                    ? false
                    : (Math.abs(hoverBackgroundTargetX - nextTargetX) > 0.5
                       || Math.abs(hoverBackgroundTargetY - nextTargetY) > 0.5)

                if (!targetChanged && !positionChanged) {
                    return
                }

                const hadTarget = hoverTargetButton !== null
                hoverBackgroundPositionAnimationEnabled = !!animatePosition && hadTarget && nextTarget !== null && (targetChanged || positionChanged)
                if (nextTarget) {
                    hoverBackgroundTargetX = nextTargetX
                    hoverBackgroundTargetY = nextTargetY
                }

                hoverTargetButton = nextTarget
            }

            function scheduleHoverSync() {
                if (hoverSyncPending) {
                    return
                }

                hoverSyncPending = true
                Qt.callLater(function() {
                    hoverSyncPending = false
                    syncHoverTarget()
                })
            }

            function syncHoverTarget() {
                let nextTarget = null
                for (let index = 0; index < gridRepeater.count; ++index) {
                    const candidate = gridRepeater.itemAt(index)
                    if (candidate && candidate.hoverActive) {
                        nextTarget = candidate
                        break
                    }
                }

                setHoverTarget(nextTarget, true)
            }

            function syncSelectionTarget() {
                if (!root.keyboardSelectionActive || root.keyboardCurrentIndex < 0 || root.keyboardCurrentIndex >= gridRepeater.count) {
                    selectionTargetButton = null
                    return
                }

                selectionTargetButton = gridRepeater.itemAt(root.keyboardCurrentIndex)
            }

            function scrollToEntry(entryIndex, animated) {
                if (entryIndex < 0 || entryIndex >= root.entries.length) {
                    return
                }

                const row = Math.floor(entryIndex / root.columnCount)
                const rowHeight = root.cellHeight + root.gridSpacing
                const targetContentY = root.clampContentY(gridFlickable,
                                                          row * rowHeight - Math.max(0, Math.round((gridFlickable.height - root.cellHeight) / 2)))

                if (!animated) {
                    scrollToEntryAnimation.stop()
                    gridFlickable.contentY = targetContentY
                    return
                }

                scrollToEntryAnimation.from = gridFlickable.contentY
                scrollToEntryAnimation.to = targetContentY
                scrollToEntryAnimation.restart()
            }

            D.ScrollView {
                id: gridScrollView
                anchors.fill: parent
                padding: 0
                clip: true

                Flickable {
                    id: gridFlickable
                    anchors.fill: parent
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

                    onContentHeightChanged: {
                        const clampedContentY = root.clampContentY(gridFlickable, contentY)
                        if (Math.abs(clampedContentY - contentY) > 0.5) {
                            scrollToEntryAnimation.stop()
                            contentY = clampedContentY
                        }
                    }
                    onHeightChanged: {
                        const clampedContentY = root.clampContentY(gridFlickable, contentY)
                        if (Math.abs(clampedContentY - contentY) > 0.5) {
                            scrollToEntryAnimation.stop()
                            contentY = clampedContentY
                        }
                    }

                    NumberAnimation {
                        id: scrollToEntryAnimation
                        target: gridFlickable
                        property: "contentY"
                        duration: 220
                        easing.type: Easing.OutCubic
                    }

                    Item {
                        id: keyboardSelectionBackground
                        x: gridContentRoot.selectionTargetButton ?
                               contentGrid.x + gridContentRoot.selectionTargetButton.x +
                               Math.round((gridContentRoot.selectionTargetButton.width - width) / 2) :
                               0
                        y: gridContentRoot.selectionTargetButton ?
                               contentGrid.y + gridContentRoot.selectionTargetButton.y :
                               0
                        width: gridContentRoot.selectionTargetButton ? gridContentRoot.selectionTargetButton.hoverWidth : 0
                        height: gridContentRoot.selectionTargetButton ? gridContentRoot.selectionTargetButton.hoverHeight : 0
                        opacity: gridContentRoot.selectionTargetButton ? 1.0 : 0.0

                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: root.selectionFillColor
                        }

                        D.InsideBoxBorder {
                            anchors.fill: parent
                            radius: 12
                            color: root.selectionInsideBorderColor
                            borderWidth: 1 / Screen.devicePixelRatio
                        }

                        D.OutsideBoxBorder {
                            anchors.fill: parent
                            radius: 12
                            color: root.selectionOutsideBorderColor
                            borderWidth: 1 / Screen.devicePixelRatio
                        }

                        Behavior on x {
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                        Behavior on y {
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                        Behavior on opacity {
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                    }

                    Item {
                        id: hoverBackground
                        x: gridContentRoot.hoverBackgroundTargetX
                        y: gridContentRoot.hoverBackgroundTargetY
                        width: gridContentRoot.hoverTargetButton ? gridContentRoot.hoverTargetButton.hoverWidth : 0
                        height: gridContentRoot.hoverTargetButton ? gridContentRoot.hoverTargetButton.hoverHeight : 0
                        opacity: gridContentRoot.hoverTargetButton && gridContentRoot.hoverTargetButton !== gridContentRoot.selectionTargetButton ? 1.0 : 0.0

                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: root.hoverFillColor
                        }

                        D.InsideBoxBorder {
                            anchors.fill: parent
                            radius: 12
                            color: root.hoverInsideBorderColor
                            borderWidth: 1 / Screen.devicePixelRatio
                        }

                        D.OutsideBoxBorder {
                            anchors.fill: parent
                            radius: 12
                            color: root.hoverOutsideBorderColor
                            borderWidth: 1 / Screen.devicePixelRatio
                        }

                        Behavior on x {
                            enabled: gridContentRoot.hoverBackgroundPositionAnimationEnabled
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                        Behavior on y {
                            enabled: gridContentRoot.hoverBackgroundPositionAnimationEnabled
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                        Behavior on opacity {
                            NumberAnimation { duration: 72; easing.type: Easing.OutQuad }
                        }
                    }

                    Grid {
                        id: contentGrid
                        x: Math.round((root.gridAreaWidth - width) / 2)
                        columns: root.columnCount
                        spacing: root.gridSpacing

                        Repeater {
                            id: gridRepeater
                            model: root.entries

                            delegate: D.ToolButton {
                                id: gridButton
                                required property int index
                                required property var modelData
                                readonly property string entryUrl: modelData && modelData.entryUrl ? String(modelData.entryUrl) : ""
                                readonly property string thumbnailUrl: modelData && modelData.thumbnailUrl ? String(modelData.thumbnailUrl) : ""
                                readonly property bool thumbnailAvailable: thumbnailUrl !== ""
                                readonly property bool thumbnailReady: thumbnailAvailable && thumbnailImage.status === Image.Ready
                                readonly property real thumbnailAspectRatio: thumbnailImage.implicitHeight > 0 ?
                                                                               thumbnailImage.implicitWidth / thumbnailImage.implicitHeight :
                                                                               1
                                readonly property int thumbnailDisplayWidth: Math.max(1, Math.round(thumbnailAspectRatio >= 1 ?
                                                                                                          root.itemIconSize :
                                                                                                          root.itemIconSize * thumbnailAspectRatio))
                                readonly property int thumbnailDisplayHeight: Math.max(1, Math.round(thumbnailAspectRatio >= 1 ?
                                                                                                           root.itemIconSize / thumbnailAspectRatio :
                                                                                                           root.itemIconSize))
                            readonly property bool keyboardSelected: root.keyboardSelectionActive && index === root.keyboardCurrentIndex
                            readonly property bool hoverActive: itemHoverHandler.hovered || down
                            property string dragImageSource: ""
                            property string dragImagePath: ""
                            property bool suppressClick: false
                            readonly property color dragTitleBackgroundColor: root.colorTheme === Dock.Dark ?
                                                                                 Qt.rgba(0, 0, 0, 0.58) :
                                                                                 Qt.rgba(1, 1, 1, 0.92)
                            readonly property color dragTitleBorderColor: root.colorTheme === Dock.Dark ?
                                                                             Qt.rgba(1, 1, 1, 0.12) :
                                                                             Qt.rgba(0, 0, 0, 0.10)
                            readonly property int dragTitleLineCount: Math.max(1, Math.min(titleMeasure.lineCount > 0 ? titleMeasure.lineCount : 1, 2))
                            readonly property int dragTitleWidth: Math.min(root.itemTextMaxWidth, Math.max(titleMetrics.advanceWidth, 24))
                            readonly property int dragTitleHeight: dragTitleLineCount * titleFontMetrics.height
                            readonly property int dragOverlayWidth: Math.max(root.itemIconSize + 24, dragTitleWidth + 18)
                            readonly property int dragOverlayHeight: root.itemIconSize + 8 + dragTitleHeight + 10
                            readonly property real sourceDragHotSpotX: width / 2
                            readonly property real sourceDragHotSpotY: root.itemTopPadding + root.itemIconSize / 2
                            readonly property real dragOverlayHotSpotScaleY: dragOverlayHeight > 0
                                                                              ? (root.itemIconSize / 2) / dragOverlayHeight
                                                                              : 0.5

                            property int hoverWidth: Math.min(root.cellWidth - 4,
                                                              Math.max(root.itemIconSize + root.itemHoverPadding * 2,
                                                                       titleMetrics.advanceWidth + root.itemHoverPadding * 2))
                            property int hoverHeight: Math.min(root.cellHeight - 2,
                                                               contentColumn.implicitHeight + root.itemHoverPadding + root.itemTextBottomMargin - root.itemHoverBottomMargin)

                            width: root.cellWidth
                            height: root.cellHeight
                            flat: true
                            padding: 0
                            Drag.dragType: Drag.Automatic
                            Drag.hotSpot.x: Qt.platform.pluginName === "xcb" ? 0 : sourceDragHotSpotX
                            Drag.hotSpot.y: Qt.platform.pluginName === "xcb" ? 0 : sourceDragHotSpotY
                            Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
                            Drag.mimeData: ({
                                "text/uri-list": gridButton.entryUrl
                            })
                            DQuickDrag.hotSpotScale: Qt.size(0.5, dragOverlayHotSpotScaleY)
                            DQuickDrag.active: Drag.active && Qt.platform.pluginName === "xcb"
                            DQuickDrag.overlay: dragOverlayWindow

                            function releaseDragImage() {
                                if (dragImagePath !== "" && root.applet && root.applet.releaseManagedTempFile) {
                                    root.applet.releaseManagedTempFile(dragImagePath)
                                }
                                dragImagePath = ""
                                dragImageSource = ""
                                Drag.imageSource = ""
                            }

                            background: Item {}

                            HoverHandler {
                                id: itemHoverHandler
                                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                                onHoveredChanged: {
                                    if (hovered || gridButton.down) {
                                        gridContentRoot.setHoverTarget(gridButton, true)
                                    } else if (gridContentRoot.hoverTargetButton === gridButton) {
                                        gridContentRoot.scheduleHoverSync()
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

                                    Item {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: root.itemIconSize
                                        height: root.itemIconSize

                                        Image {
                                            id: thumbnailImage
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            source: gridButton.thumbnailUrl
                                            sourceSize: Qt.size(Math.round(root.itemIconSize * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)),
                                                                Math.round(root.itemIconSize * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)))
                                            fillMode: Image.Stretch
                                            asynchronous: true
                                            cache: false
                                            smooth: true
                                            visible: false
                                        }

                                        Rectangle {
                                            id: thumbnailMask
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: "white"
                                            visible: false
                                        }

                                        OpacityMask {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            source: thumbnailImage
                                            maskSource: thumbnailMask
                                            cached: false
                                            visible: gridButton.thumbnailReady
                                        }

                                        D.InsideBoxBorder {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: root.thumbnailInsideBorderColor
                                            borderWidth: 1 / Screen.devicePixelRatio
                                            visible: gridButton.thumbnailReady
                                        }

                                        D.OutsideBoxBorder {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: root.thumbnailOutsideBorderColor
                                            borderWidth: 1 / Screen.devicePixelRatio
                                            visible: gridButton.thumbnailReady
                                        }

                                        D.DciIcon {
                                            anchors.centerIn: parent
                                            width: root.itemIconSize
                                            height: root.itemIconSize
                                            sourceSize: Qt.size(width, height)
                                            name: modelData.iconName
                                            smooth: false
                                            retainWhileLoading: true
                                            visible: !gridButton.thumbnailReady
                                        }
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
                                        color: root.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 1) : Qt.rgba(0, 0, 0, 1)
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

                            property Component dragOverlayWindow: QuickDragWindow {
                                width: gridButton.dragOverlayWidth
                                height: gridButton.dragOverlayHeight

                                Column {
                                    id: dragVisual
                                    anchors.centerIn: parent
                                    spacing: 8
                                    width: gridButton.dragOverlayWidth

                                    Item {
                                        id: dragIconPreview
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        implicitWidth: root.itemIconSize
                                        implicitHeight: root.itemIconSize

                                        Image {
                                            id: dragThumbnailImage
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            source: gridButton.thumbnailUrl
                                            sourceSize: Qt.size(Math.round(root.itemIconSize * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)),
                                                                Math.round(root.itemIconSize * (Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0)))
                                            fillMode: Image.Stretch
                                            asynchronous: true
                                            cache: false
                                            smooth: true
                                            visible: false
                                        }

                                        Rectangle {
                                            id: dragThumbnailMask
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: "white"
                                            visible: false
                                        }

                                        OpacityMask {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            source: dragThumbnailImage
                                            maskSource: dragThumbnailMask
                                            cached: false
                                            visible: gridButton.thumbnailAvailable && dragThumbnailImage.status === Image.Ready
                                        }

                                        D.InsideBoxBorder {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: root.thumbnailInsideBorderColor
                                            borderWidth: 1 / Screen.devicePixelRatio
                                            visible: gridButton.thumbnailAvailable && dragThumbnailImage.status === Image.Ready
                                        }

                                        D.OutsideBoxBorder {
                                            anchors.centerIn: parent
                                            width: gridButton.thumbnailDisplayWidth
                                            height: gridButton.thumbnailDisplayHeight
                                            radius: root.thumbnailCornerRadius
                                            color: root.thumbnailOutsideBorderColor
                                            borderWidth: 1 / Screen.devicePixelRatio
                                            visible: gridButton.thumbnailAvailable && dragThumbnailImage.status === Image.Ready
                                        }

                                        D.DciIcon {
                                            anchors.centerIn: parent
                                            width: root.itemIconSize
                                            height: root.itemIconSize
                                            sourceSize: Qt.size(width, height)
                                            name: gridButton.modelData && gridButton.modelData.iconName ? String(gridButton.modelData.iconName) : ""
                                            visible: !gridButton.thumbnailAvailable || dragThumbnailImage.status !== Image.Ready
                                            smooth: false
                                            retainWhileLoading: true
                                        }
                                    }

                                    Rectangle {
                                        id: dragTitleBackground
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        implicitWidth: gridButton.dragOverlayWidth
                                        implicitHeight: gridButton.dragTitleHeight + 10
                                        radius: 8
                                        color: gridButton.dragTitleBackgroundColor

                                        D.InsideBoxBorder {
                                            anchors.fill: parent
                                            radius: parent.radius
                                            color: gridButton.dragTitleBorderColor
                                            borderWidth: 1 / Screen.devicePixelRatio
                                        }

                                        Label {
                                            id: dragTitleLabel
                                            anchors.centerIn: parent
                                            width: Math.min(root.itemTextMaxWidth, parent.width - 12)
                                            horizontalAlignment: Text.AlignHCenter
                                            wrapMode: Text.WrapAnywhere
                                            maximumLineCount: 2
                                            elide: Text.ElideNone
                                            text: root.twoLineMiddleElidedText(titleFontMetrics,
                                                                               titleMeasure.text,
                                                                               width,
                                                                               titleMeasure.lineCount > 2)
                                            color: root.colorTheme === Dock.Dark ? Qt.rgba(1, 1, 1, 1) : Qt.rgba(0, 0, 0, 1)
                                            font.pixelSize: 11
                                            font.weight: Font.Normal
                                        }
                                    }
                                }
                            }

                            Timer {
                                id: suppressClickTimer
                                interval: 120
                                repeat: false
                                onTriggered: {
                                    gridButton.suppressClick = false
                                }
                            }

                            DragHandler {
                                id: fileDragHandler
                                target: null
                                enabled: gridButton.entryUrl !== ""
                                acceptedButtons: Qt.LeftButton
                                dragThreshold: 6
                                onActiveChanged: {
                                    if (active) {
                                        gridButton.suppressClick = true
                                        Panel.contextDragging = true
                                        gridButton.releaseDragImage()
                                        if (Qt.platform.pluginName !== "xcb") {
                                            gridButton.grabToImage(function(result) {
                                                if (!fileDragHandler.active) {
                                                    return
                                                }

                                                const dragImagePath = root.applet && root.applet.createManagedTempFilePath
                                                                      ? root.applet.createManagedTempFilePath("dock-popup-drag-", ".png")
                                                                      : ""
                                                if (dragImagePath === "") {
                                                    return
                                                }
                                                if (!result.saveToFile(dragImagePath)) {
                                                    if (root.applet && root.applet.releaseManagedTempFile) {
                                                        root.applet.releaseManagedTempFile(dragImagePath)
                                                    }
                                                    return
                                                }
                                                const dragImageUrl = "file://" + dragImagePath
                                                gridButton.dragImagePath = dragImagePath
                                                gridButton.dragImageSource = dragImageUrl
                                                gridButton.Drag.imageSource = dragImageUrl
                                            })
                                        }
                                    } else {
                                        Panel.contextDragging = false
                                        gridButton.releaseDragImage()
                                        suppressClickTimer.restart()
                                    }

                                    Qt.callLater(function() {
                                        gridButton.Drag.active = fileDragHandler.active
                                        if (fileDragHandler.active) {
                                            root.closeRequested()
                                        }
                                    })
                                }
                            }
                            Component.onDestruction: releaseDragImage()

                            onClicked: {
                                if (gridButton.suppressClick || fileDragHandler.active || gridButton.Drag.active) {
                                    return
                                }

                                root.selectEntryIndex(index, false)
                                if (modelData.directory) {
                                    root.refresh(modelData.entryId, true)
                                    return
                                }

                                root.applet.activatePopupEntry(root.dockElement, modelData.entryId)
                                root.closeRequested()
                            }

                            hoverEnabled: false

                            onDownChanged: {
                                if (down) {
                                    gridContentRoot.setHoverTarget(gridButton, true)
                                } else if (!itemHoverHandler.hovered && gridContentRoot.hoverTargetButton === gridButton) {
                                    gridContentRoot.scheduleHoverSync()
                                }
                            }
                            onKeyboardSelectedChanged: gridContentRoot.syncSelectionTarget()
                            Component.onCompleted: {
                                gridContentRoot.scheduleHoverSync()
                                gridContentRoot.syncSelectionTarget()
                            }
                        }

                        onItemAdded: function(index, item) {
                            gridContentRoot.scheduleHoverSync()
                            gridContentRoot.syncSelectionTarget()
                        }

                        onItemRemoved: function(index, item) {
                            gridContentRoot.scheduleHoverSync()
                            gridContentRoot.syncSelectionTarget()
                        }
                    }
                }
            }
            }

            Connections {
                target: root
                function onKeyboardCurrentIndexChanged() {
                    gridContentRoot.syncSelectionTarget()
                }
                function onKeyboardSelectionActiveChanged() {
                    gridContentRoot.syncSelectionTarget()
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
                root.resetKeyboardNavigation()
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
