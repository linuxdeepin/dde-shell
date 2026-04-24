// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1 as LP

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root
    required property int displayMode
    required property int colorTheme
    required property bool itemActive
    required property bool attention
    required property string itemId
    required property string dockElement
    required property string itemKind
    required property string name
    required property string iconName
    required property var previewIcons
    required property string menus
    required property list<string> windows
    required property int visualIndex
    required property var modelIndex
    required property string title

    property real blendOpacity: 1.0
    property point lastSpotlightPoint: Qt.point(0, 0)
    readonly property string toolTipText: root.itemId === "dde-trash"
                                          ? root.name + "-" + taskmanager.Applet.trashTipText
                                          : root.name

    signal dropFilesOnItem(itemId: string, files: list<string>)
    signal dragFinished()

    Drag.active: mouseArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: icon.width / 2
    Drag.hotSpot.y: icon.height / 2
    Drag.dragType: Drag.Automatic
    Drag.mimeData: {
        "text/x-dde-dock-dnd-appid": itemId,
        "text/x-dde-dock-dnd-element": dockElement,
        "text/x-dde-dock-dnd-itemkind": itemKind,
        "text/x-dde-dock-dnd-source": "taskbar",
        "text/x-dde-dock-dnd-winid": windows.length > 0 ? windows[0] : ""
    }
    
    property bool useColumnLayout: Panel.rootObject.useColumnLayout
    property bool compactFashionIndicator: root.displayMode === Dock.Fashion && Panel.position === Dock.Bottom
    property int statusIndicatorSize: useColumnLayout ? root.width * 0.72 : root.height * 0.72
    property int iconSize: Panel.rootObject.dockItemMaxSize * 9 / 14
    property int popupIconSize: Math.max(1, Math.round(iconSize * 0.92))
    property bool enableTitle: false
    property bool titleActive: enableTitle && titleLoader.active
    property int appTitleSpacing: 0
    property bool popupItem: root.itemKind === "group" || root.itemKind === "folder"
    property var iconGlobalPoint: {
        var a = icon
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x, y)
    }

    implicitWidth: appItem.implicitWidth

    function mapSpotlightPoint(localPoint) {
        const point = localPoint || Qt.point(appItem.width / 2, appItem.height / 2)
        return appItem.mapToItem(null, point.x, point.y)
    }

    function updateSpotlight(localPoint) {
        lastSpotlightPoint = mapSpotlightPoint(localPoint)
        Panel.reportMousePresence(true, lastSpotlightPoint)
    }

    function clearSpotlight() {
        Panel.reportMousePresence(false, lastSpotlightPoint)
    }

    // Monitor Panel position changes to update icon geometry
    Connections {
        target: Panel.rootObject
        function onXChanged() {
            updateWindowIconGeometryTimer.start()
        }
        function onYChanged() {
            updateWindowIconGeometryTimer.start()
        }
    }

    AppItemPalette {
        id: itemPalette
        displayMode: root.displayMode
        colorTheme: root.colorTheme
        itemActive: root.itemActive
        backgroundColor: D.DTK.palette.highlight
    }

    Control {
        anchors.fill: parent
        id: appItem
        implicitWidth: root.titleActive ? (iconContainer.width + 4 + titleLoader.width + root.appTitleSpacing) : iconContainer.width + root.appTitleSpacing
        visible: !root.Drag.active // When in dragging, hide app item
        background: AppletItemBackground {
            id: hoverBackground

            readonly property int hoverInset: 4
            readonly property int verticalSpacing: hoverInset
            readonly property int horizontalSpacing: hoverInset
            readonly property int nonSplitHeight: root.iconSize + verticalSpacing * 2
            readonly property int splitWidth: Math.round(icon.width
                                                         + titleLoader.anchors.leftMargin
                                                         + titleLoader.width
                                                         + horizontalSpacing * 2)
            readonly property int nonSplitWidth: Math.round(root.iconSize + horizontalSpacing * 2)

            enabled: false

            width: root.titleActive ? splitWidth : nonSplitWidth
            height: nonSplitHeight
            radius: height / 5
            anchors.centerIn: parent
            isActive: root.itemActive
            opacity: (hoverHandler.hovered || (root.itemActive && root.windows.length > 0)) ? 1.0 : 0.0
            D.ColorSelector.hovered: hoverHandler.hovered
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
        Item {
            id: iconContainer
            width: root.iconSize 
            height: parent.height
            anchors.verticalCenter: parent.verticalCenter
            states: [
                State {
                    name: "titleActive"
                    when: root.titleActive

                    AnchorChanges {
                        target: iconContainer
                        anchors.left: parent.left
                        anchors.horizontalCenter: undefined
                    }
                    PropertyChanges {
                        target: iconContainer
                        anchors.leftMargin: hoverBackground.horizontalSpacing
                    }
                },
                State {
                    name: "nonTitleActive"
                    when: !root.titleActive

                    AnchorChanges {
                        target: iconContainer
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.left: undefined
                    }
                }
            ]
            StatusIndicator {
                id: statusIndicator
                palette: itemPalette
                width: root.statusIndicatorSize
                height: root.statusIndicatorSize
                anchors.centerIn: iconContainer
                visible: root.displayMode === Dock.Efficient && root.windows.length > 0
            }

            Connections {
                function onPositionChanged() {
                    windowIndicator.updateIndicatorAnchors()
                    updateWindowIconGeometryTimer.start()
                }
                target: Panel
            }

            D.DciIcon {
                id: icon
                name: root.iconName
                height: iconSize
                width: iconSize
                sourceSize: Qt.size(iconSize, iconSize)
                anchors.centerIn: parent
                retainWhileLoading: true
                smooth: false
                visible: !root.popupItem

                function mapToScene(px, py) {
                    return parent.mapToItem(Window.window.contentItem, Qt.point(px, py))
                }

                function mapFromScene(px, py) {
                    return parent.mapFromItem(Window.window.contentItem, Qt.point(px, py))
                }

                function fixPosition() {
                    if (root.Drag.active || !parent || launchAnimation.running) {
                        return
                    }
                    anchors.centerIn = undefined
                    var targetX = (parent.width - width) / 2
                    var targetY = (parent.height - height) / 2

                    var scenePos = mapToScene(targetX, targetY)
                    
                    var physicalX = Math.round(scenePos.x * Panel.devicePixelRatio)
                    var physicalY = Math.round(scenePos.y * Panel.devicePixelRatio)

                    var localPos = mapFromScene(physicalX / Panel.devicePixelRatio, physicalY / Panel.devicePixelRatio)
                    
                    x = localPos.x
                    y = localPos.y
                }

                Timer {
                    id: fixPositionTimer
                    interval: 100
                    repeat: false
                    running: false
                    onTriggered: {
                        icon.fixPosition()
                    }
                }

                Connections {
                    target: root
                    function onIconGlobalPointChanged() {
                        fixPositionTimer.start()
                    }
                }
                LaunchAnimation {
                    id: launchAnimation
                    launchSpace: {
                        switch (Panel.position) {
                        case Dock.Top:
                        case Dock.Bottom:
                            return (root.height - icon.height) / 2
                        case Dock.Left:
                        case Dock.Right:
                            return (root.width - icon.width) / 2
                        }
                    }

                    direction: {
                        switch (Panel.position) {
                        case Dock.Top:
                            return LaunchAnimation.Direction.Down
                        case Dock.Bottom:
                            return LaunchAnimation.Direction.Up
                        case Dock.Left:
                            return LaunchAnimation.Direction.Right
                        case Dock.Right:
                            return LaunchAnimation.Direction.Left
                        }
                    }
                    target: icon
                    loops: 1
                    running: false
                }
            }

            PinnedItemIcon {
                anchors.centerIn: parent
                width: root.popupIconSize
                height: root.popupIconSize
                iconName: root.iconName
                previewIcons: root.previewIcons
                iconSize: root.popupIconSize
                colorTheme: root.colorTheme
                visible: root.popupItem
            }
        }

        WindowIndicator {
            id: windowIndicator
            dotWidth: root.compactFashionIndicator ? 4 : (root.useColumnLayout ? Math.max(iconSize / 16, 2) : Math.max(iconSize / 3, 2))
            dotHeight: root.compactFashionIndicator ? 3 : (root.useColumnLayout ? Math.max(iconSize / 3, 2) : Math.max(iconSize / 16, 2))
            multiDotWidth: root.compactFashionIndicator ? 3 : (root.useColumnLayout ? Math.max(iconSize / 16, 2) : Math.max(iconSize / 3, 2))
            multiDotHeight: root.compactFashionIndicator ? 3 : (root.useColumnLayout ? Math.max(iconSize / 3, 2) : Math.max(iconSize / 16, 2))
            windows: root.windows
            displayMode: root.displayMode
            useColumnLayout: root.useColumnLayout
            compactFashionIndicator: root.compactFashionIndicator
            palette: itemPalette
            visible: (root.displayMode === Dock.Efficient && root.windows.length > 1) || (root.displayMode === Dock.Fashion && root.windows.length > 0)

            function updateIndicatorAnchors() {
                windowIndicator.anchors.top = undefined
                windowIndicator.anchors.topMargin = 0
                windowIndicator.anchors.bottom = undefined
                windowIndicator.anchors.bottomMargin = 0
                windowIndicator.anchors.left = undefined
                windowIndicator.anchors.leftMargin = 0
                windowIndicator.anchors.right = undefined
                windowIndicator.anchors.rightMargin = 0
                windowIndicator.anchors.horizontalCenter = undefined
                windowIndicator.anchors.verticalCenter = undefined

                switch(Panel.position) {
                case Dock.Top: {
                    windowIndicator.anchors.horizontalCenter = iconContainer.horizontalCenter
                    windowIndicator.anchors.top = hoverBackground.top
                    windowIndicator.anchors.topMargin = 1
                    return
                }
                case Dock.Bottom: {
                    windowIndicator.anchors.horizontalCenter = iconContainer.horizontalCenter
                    windowIndicator.anchors.bottom = root.compactFashionIndicator ? appItem.bottom : hoverBackground.bottom
                    windowIndicator.anchors.bottomMargin = 1
                    return
                }
                case Dock.Left: {
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.left = hoverBackground.left
                    windowIndicator.anchors.leftMargin = 1
                    return
                }
                case Dock.Right:{
                    windowIndicator.anchors.verticalCenter = parent.verticalCenter
                    windowIndicator.anchors.right = hoverBackground.right
                    windowIndicator.anchors.rightMargin = 1
                    return
                }
                }
            }

            Component.onCompleted: {
                windowIndicator.updateIndicatorAnchors()
            }

            Connections {
                target: root
                function onCompactFashionIndicatorChanged() {
                    windowIndicator.updateIndicatorAnchors()
                }
            }
        }

        AppItemTitle {
            id: titleLoader
            anchors.left: iconContainer.right
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            enabled: root.enableTitle && root.windows.length > 0
            text: root.title
            colorTheme: root.colorTheme
        }

        // TODO: value can set during debugPanel
        Loader {
            id: animationRoot
            anchors.fill: parent
            z: -1
            active: root.attention && !Panel.rootObject.isDragging
            sourceComponent: Repeater {
                model: 5
                Rectangle {
                    id: rect
                    required property int index
                    property var originSize: iconSize

                    width: originSize * (index - 1)
                    height: width
                    radius: width / 2
                    color: Qt.rgba(1, 1, 1, 0.1)

                    anchors.centerIn: parent
                    opacity: Math.min(3 - width / originSize, root.blendOpacity)

                    SequentialAnimation {
                        running: true
                        loops: Animation.Infinite

                        // 弹出
                        ParallelAnimation {
                            NumberAnimation { target: rect; property: "width"; from: Math.max(originSize * (index - 1), 0); to: originSize * (index); duration: 1200 }
                            ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                            NumberAnimation { target: icon; property: "scale"; from: 1.0; to: 1.15; duration: 1200; easing.type: Easing.OutElastic; easing.amplitude: 1; easing.period: 0.2 }
                        }

                        // 收缩
                        ParallelAnimation {
                            NumberAnimation { target: rect; property: "width"; from: originSize * (index); to: originSize * (index + 1); duration: 1200 }
                            ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                            NumberAnimation { target: icon; property: "scale"; from: 1.15; to: 1.0; duration: 1200; easing.type: Easing.OutElastic; easing.amplitude: 1; easing.period: 0.2 }
                        }

                        // 停顿
                        ParallelAnimation {
                            NumberAnimation { target: rect; property: "width"; from: originSize * (index + 1); to: originSize * (index + 2); duration: 1200 }
                            ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                        }
                    }

                    // TODO Remove it because of consuming performance.
                    // D.BoxShadow {
                    //     visible: rect.visible
                    //     anchors.fill: rect
                    //     z: -2
                    //     shadowBlur: 20
                    //     shadowColor : Qt.rgba(0, 0, 0, 0.05)
                    //     shadowOffsetX : 0
                    //     shadowOffsetY : 0
                    //     cornerRadius: rect.radius
                    //     hollow: true
                    // }
                }
            }
        }

        HoverHandler {
            id: hoverHandler
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus

            onPointChanged: {
                if (hovered) {
                    appItemSpotlightClearTimer.stop()
                    root.updateSpotlight(hoverHandler.point.position)
                }
            }

            onHoveredChanged: function () {
                if (hovered) {
                    appItemSpotlightClearTimer.stop()
                    root.updateSpotlight()
                    root.onEntered()
                } else {
                    appItemSpotlightClearTimer.restart()
                    root.onExited()
                }
            }
        }
    }

    Timer {
        id: appItemSpotlightClearTimer
        interval: 70
        repeat: false
        onTriggered: {
            if (!hoverHandler.hovered) {
                root.clearSpotlight()
            }
        }
    }

    Loader {
        id: contextMenuLoader
        active: false
        property bool trashEmpty: true
        sourceComponent: LP.Menu {
            id: contextMenu
            property var menuItems: {
                try {
                    return JSON.parse(root.menus || "[]")
                } catch (error) {
                    console.warn("failed to parse taskmanager menu", error, root.menus)
                    return []
                }
            }
            property var popupSortInfo: ({})
            readonly property bool preferChineseText: {
                const localeName = String(Qt.locale().name || "").toLowerCase()
                const uiLanguage = String(Qt.uiLanguage || "").toLowerCase()
                return localeName.indexOf("zh") === 0 || uiLanguage.indexOf("zh") === 0
            }

            function localizedMenuText(text) {
                const sourceText = String(text || "")
                if (!preferChineseText || !sourceText.length) {
                    return sourceText
                }

                const textMap = {
                    "Open": "打开",
                    "Undock": "移除驻留",
                    "Dock": "驻留",
                    "Force Quit": "强制退出",
                    "Close All": "关闭所有",
                    "Close this window": "关闭当前窗口",
                    "Open in File Manager": "在文件管理器中打开",
                    "Move to Trash": "移动到回收站",
                    "Sort": "排序方式",
                    "Ascending": "升序",
                    "Descending": "降序",
                    "Name": "名称",
                    "Modified Time": "修改时间",
                    "Created Time": "创建时间",
                    "Size": "大小",
                    "Type": "类型",
                    "Sort by Name": "按名称排序",
                    "Sort by Modified Time": "按修改时间排序",
                    "Sort by Created Time": "按创建时间排序",
                    "Sort by Size": "按大小排序",
                    "Sort by Type": "按类型排序"
                }

                return textMap[sourceText] !== undefined ? textMap[sourceText] : sourceText
            }

            function refreshPopupSortInfo() {
                popupSortInfo = root.popupItem ? TaskManager.popupSortState(root.dockElement) : ({})
            }

            function isCurrentPopupSort(fieldName) {
                return popupSortInfo && popupSortInfo.sortField === fieldName
            }

            function popupSortText(baseText, fieldName) {
                if (!isCurrentPopupSort(fieldName)) {
                    return localizedMenuText(baseText)
                }

                const localizedBaseText = localizedMenuText(baseText)
                return localizedBaseText + (popupSortInfo.sortDescending ?
                                                " (" + localizedMenuText(qsTr("Descending")) + ")" :
                                                " (" + localizedMenuText(qsTr("Ascending")) + ")")
            }

            function applyPopupSort(fieldName) {
                TaskManager.cyclePopupSort(root.dockElement, fieldName)
                refreshPopupSortInfo()
                if (pinnedPopup.popupVisible) {
                    pinnedPopupContent.refresh(pinnedPopupContent.currentLocation(), false)
                }
            }

            Component.onCompleted: refreshPopupSortInfo()
            onAboutToShow: refreshPopupSortInfo()
            Instantiator {
                id: menuItemInstantiator
                model: contextMenu.menuItems
                delegate: LP.MenuItem {
                    required property var modelData

                    readonly property string menuId: modelData && modelData.id !== undefined ? String(modelData.id) : ""
                    readonly property string menuText: modelData && modelData.name !== undefined ? String(modelData.name) : ""

                    text: contextMenu.localizedMenuText(menuText)
                    enabled: (root.itemId === "dde-trash" && menuId === "clean-trash")
                            ? !contextMenuLoader.trashEmpty
                            : true
                    onTriggered: {
                        TaskManager.requestNewInstance(root.modelIndex, menuId);
                    }
                }
                onObjectAdded: (index, object) => contextMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => contextMenu.removeItem(object)
            }
            Instantiator {
                id: popupSortMenuInstantiator
                model: root.popupItem ? 1 : 0
                delegate: LP.Menu {
                    title: contextMenu.localizedMenuText(qsTr("Sort"))

                    LP.MenuItem {
                        checkable: true
                        text: contextMenu.popupSortText(qsTr("Name"), "name")
                        checked: contextMenu.isCurrentPopupSort("name")
                        onTriggered: contextMenu.applyPopupSort("name")
                    }
                    LP.MenuItem {
                        checkable: true
                        text: contextMenu.popupSortText(qsTr("Modified Time"), "modified")
                        checked: contextMenu.isCurrentPopupSort("modified")
                        onTriggered: contextMenu.applyPopupSort("modified")
                    }
                    LP.MenuItem {
                        checkable: true
                        text: contextMenu.popupSortText(qsTr("Created Time"), "created")
                        checked: contextMenu.isCurrentPopupSort("created")
                        onTriggered: contextMenu.applyPopupSort("created")
                    }
                    LP.MenuItem {
                        checkable: true
                        text: contextMenu.popupSortText(qsTr("Size"), "size")
                        checked: contextMenu.isCurrentPopupSort("size")
                        onTriggered: contextMenu.applyPopupSort("size")
                    }
                    LP.MenuItem {
                        checkable: true
                        text: contextMenu.popupSortText(qsTr("Type"), "type")
                        checked: contextMenu.isCurrentPopupSort("type")
                        onTriggered: contextMenu.applyPopupSort("type")
                    }
                }
                onObjectAdded: (index, object) => contextMenu.insertMenu(contextMenu.items.length, object)
                onObjectRemoved: (index, object) => contextMenu.removeMenu(object)
            }
        }
    }

    PanelPopup {
        id: pinnedPopup
        property point popupAnchorPoint: Qt.point(0, 0)
        width: pinnedPopupContent.width
        height: pinnedPopupContent.height
        popupX: {
            switch (Panel.position) {
            case Dock.Top:
            case Dock.Bottom:
                return popupAnchorPoint.x - pinnedPopup.width / 2
            case Dock.Right:
                return -pinnedPopup.width - 10
            case Dock.Left:
                return Panel.rootObject.dockSize + 10
            }
            return popupAnchorPoint.x - pinnedPopup.width / 2
        }
        popupY: {
            switch (Panel.position) {
            case Dock.Top:
                return Panel.rootObject.dockSize + 10
            case Dock.Right:
            case Dock.Left:
                return popupAnchorPoint.y - pinnedPopup.height / 2
            case Dock.Bottom:
                return -pinnedPopup.height - 10
            }
            return -pinnedPopup.height - 10
        }

        DockPinnedPopup {
            id: pinnedPopupContent
            applet: taskmanager.Applet
            dockElement: root.dockElement
            colorTheme: root.colorTheme
            popupWindow: pinnedPopup.popupWindow
            onCloseRequested: pinnedPopup.close()
        }

        Component.onCompleted: {
            if ("keyEventTarget" in pinnedPopup) {
                pinnedPopup.keyEventTarget = pinnedPopupContent
            }
        }
    }

    Connections {
        target: pinnedPopup.popupWindow

        function onVisibleChanged() {
            if (!pinnedPopup.popupWindow || pinnedPopup.popupWindow.visible) {
                return
            }

            DS.grabKeyboard(pinnedPopup.popupWindow, false)
        }
    }

    Timer {
        id: pinnedPopupKeyboardGrabTimer
        interval: 1
        repeat: false
        onTriggered: {
            if (!pinnedPopup.popupWindow || !pinnedPopup.popupWindow.visible) {
                return
            }

            pinnedPopup.popupWindow.requestActivate()
            DS.grabKeyboard(pinnedPopup.popupWindow)
            pinnedPopupContent.forceActiveFocus(Qt.OtherFocusReason)
        }
    }

    Timer {
        id: updateWindowIconGeometryTimer
        interval: 500
        running: false
        repeat: false
        onTriggered: {
            var pos = icon.mapToItem(null, 0, 0)
            taskmanager.Applet.requestUpdateWindowIconGeometry(root.modelIndex, Qt.rect(pos.x, pos.y,
                icon.width, icon.height), Panel.rootObject)
        }
    }

    Timer {
        id: previewTimer
        interval: 220
        running: false
        repeat: false
        property int xOffset: 0
        property int yOffset: 0
        onTriggered: {
            if (root.popupItem) {
                return
            }

            if (root.windows.length != 0 || Qt.platform.pluginName === "wayland") {
                // 使用基于 modelIndex 的预览API，确保精确匹配
                root.requestPreviewNow(xOffset, yOffset)
            }
        }
    }

    function togglePinnedPopup() {
        if (pinnedPopup.popupVisible) {
            pinnedPopup.close()
            return
        }

        Panel.requestClosePopup()
        pinnedPopupContent.beginPopupSession()
        pinnedPopupContent.refresh("", false)
        if ("keyEventTarget" in pinnedPopup) {
            pinnedPopup.keyEventTarget = pinnedPopupContent
        }
        pinnedPopup.popupAnchorPoint = root.mapToItem(null, root.width / 2, root.height / 2)
        pinnedPopup.open()
        pinnedPopupKeyboardGrabTimer.restart()
    }

    function showToolTipNow() {
        var point = root.mapToItem(null, root.width / 2, root.height / 2)
        toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
        toolTip.open()
    }

    function requestPreviewNow(xOffset, yOffset) {
        taskmanager.Applet.requestPreview(root.modelIndex,
                                          Panel.rootObject,
                                          xOffset,
                                          yOffset,
                                          Panel.position)
    }

    function onEntered() {
        if (root.popupItem) {
            if (toolTip.toolTipWindow && toolTip.toolTipWindow.visible) {
                showToolTipNow()
            } else {
                toolTipShowTimer.start()
            }
            return
        }

        if (Qt.platform.pluginName === "xcb" && windows.length === 0) {
            if (toolTip.toolTipWindow && toolTip.toolTipWindow.visible) {
                showToolTipNow()
            } else {
                toolTipShowTimer.start()
            }
            return
        }

        var itemPos = root.mapToItem(null, 0, 0)
        let xOffset, yOffset, interval = 10
        if (Panel.position % 2 === 0) {
            xOffset = itemPos.x + (root.width / 2)
            yOffset = (Panel.position == 2 ? -interval : interval + Panel.dockSize)
        } else {
            xOffset = (Panel.position == 1 ? -interval : interval + Panel.dockSize)
            yOffset = itemPos.y + (root.height / 2)
        }
        if (root.windows.length > 0 && taskmanager.previewSwitchActive) {
            taskmanager.endPreviewSwitch()
            requestPreviewNow(xOffset, yOffset)
            return
        }
        previewTimer.xOffset = xOffset
        previewTimer.yOffset = yOffset
        previewTimer.start()
    }

    function onExited() {
        var hadPendingPreview = previewTimer.running
        if (toolTipShowTimer.running) {
            toolTipShowTimer.stop()
        }

        if (hadPendingPreview) {
            previewTimer.stop()
        }

        if (root.popupItem || (Qt.platform.pluginName === "xcb" && windows.length === 0)) {
            toolTip.close()
            return
        }
        if (root.windows.length > 0 && !hadPendingPreview) {
            taskmanager.beginPreviewSwitch()
        }
        closeItemPreview()
    }

    function closeItemPreview() {
        if (root.popupItem) {
            return
        }

        if (previewTimer.running) {
            previewTimer.stop()
        } else {
            taskmanager.Applet.hideItemPreview()
        }
    }

    function requestAppItemMenu() {
        Panel.requestClosePopup()
        contextMenuLoader.trashEmpty = taskmanager.Applet.trashEmpty
        contextMenuLoader.active = true
        MenuHelper.openMenu(contextMenuLoader.item)
    }

    function openAppItemMenu() {
        toolTip.close()
        closeItemPreview()
        requestAppItemMenu()
    }

    TapHandler {
        acceptedButtons: Qt.NoButton
        acceptedDevices: PointerDevice.TouchScreen
        onLongPressed: root.openAppItemMenu()
    }

    MouseArea {
        id: contextMenuMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        hoverEnabled: false
        preventStealing: true

        onPressed: function(mouse) {
            toolTip.close()
            closeItemPreview()
        }

        onClicked: function(mouse) {
            root.openAppItemMenu()
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: false
        acceptedButtons: Qt.LeftButton
        drag.target: root
        drag.onActiveChanged: {
            if (!drag.active) {
                Panel.contextDragging = false
                root.dragFinished()
                return
            }
            Panel.contextDragging = true
        }

        onPressed: function (mouse) {
            if (mouse.button === Qt.LeftButton) {
                appItem.grabToImage(function(result) {
                    root.Drag.imageSource = result.url;
                })
                appItemSpotlightClearTimer.stop()
            }
            toolTip.close()
            closeItemPreview()
        }
        onClicked: function (mouse) {
            let index = root.modelIndex;
            if (root.popupItem) {
                togglePinnedPopup()
                return
            }

            if (root.windows.length === 0) {
                launchAnimation.start();
                TaskManager.requestNewInstance(index, "");
                return;
            }
            TaskManager.requestActivate(index);
        }

        PanelToolTip {
            id: toolTip
            text: root.toolTipText
            toolTipX: DockPanelPositioner.x
            toolTipY: DockPanelPositioner.y
            closeGraceInterval: 90
        }

        PanelToolTip {
            id: dragToolTip
            text: qsTr("Move to Trash")
            toolTipX: DockPanelPositioner.x
            toolTipY: DockPanelPositioner.y
            visible: false
        }

        Timer {
            id: toolTipShowTimer
            interval: 50
            onTriggered: {
                root.showToolTipNow()
            }
        }
    }

    Timer {
        id: dragToolTipCloseTimer
        interval: 100
        onTriggered: {
            dragToolTip.close()
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["dfm_app_type_for_drag"]

        onEntered: function (drag) {
            if (root.itemId === "dde-trash") {
                dragToolTipCloseTimer.stop()
                if (!dragToolTip.toolTipVisible) {
                    var point = root.mapToItem(null, root.width / 2, root.height / 2)
                    dragToolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, dragToolTip.width, dragToolTip.height)
                    dragToolTip.open()
                }
            }
        }

        onExited: function (drag) {
            if (root.itemId === "dde-trash") {
                dragToolTipCloseTimer.restart()
            }
        }

        onDropped: function (drop){
            if (root.popupItem) {
                return
            }
            dragToolTipCloseTimer.stop()
            dragToolTip.close()
            root.dropFilesOnItem(root.itemId, drop.urls)
        }
    }

    onWindowsChanged: {
        updateWindowIconGeometryTimer.start()
        // Close tooltip when window appears
        if (windows.length > 0 && toolTip.toolTipVisible) {
            toolTip.close()
        }
    }

    onIconGlobalPointChanged: {
        updateWindowIconGeometryTimer.start()
    }

}
