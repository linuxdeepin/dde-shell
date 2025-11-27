// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D
import Qt.labs.platform 1.1 as LP

Item {
    id: root
    required property int displayMode
    required property int colorTheme
    required property bool active
    required property bool attention
    required property string itemId
    required property string name
    required property string windowTitle
    required property string iconName
    required property string menus
    required property list<string> windows
    required property int visualIndex
    required property var modelIndex
    property int maxCharLimit: 7

    signal dropFilesOnItem(itemId: string, files: list<string>)
    signal dragFinished()

    Drag.active: mouseArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: iconContainer.width / 2
    Drag.hotSpot.y: iconContainer.height / 2
    Drag.dragType: Drag.Automatic
    Drag.mimeData: { 
        "text/x-dde-dock-dnd-appid": itemId, 
        "text/x-dde-dock-dnd-source": "taskbar",
        "text/x-dde-dock-dnd-winid": windows.length > 0 ? windows[0] : ""
    }

    property bool useColumnLayout: Panel.position % 2
    property int statusIndicatorSize: useColumnLayout ? root.width * 0.72 : root.height * 0.72
    property int iconSize: Panel.rootObject.dockItemMaxSize * 9 / 14
    
    // 根据图标尺寸计算文字大小，最大20最小10
    property int textSize: Math.max(10, Math.min(20, Math.round(iconSize * 0.35)))

    property string displayText: {
        if (root.windows.length === 0)
            return ""

        if (!root.windowTitle || root.windowTitle.length === 0)
            return ""

        var source = root.windowTitle
        var maxChars = root.maxCharLimit

        // maxCharLimit 为 0 时不显示文字
        if (maxChars <= 0)
            return ""

        var len = source.length
        var displayLen = 0

        if (len <= maxChars) {
            displayLen = len
        } else {
            // 文本超过最大字符数时，最多显示 6 个字符，再加省略号
            displayLen = maxChars
        }

        if (displayLen <= 0)
            return ""

        if (len > maxChars) {
            return source.substring(0, displayLen) + "…"
        } else {
            return source.substring(0, displayLen)
        }
    }
    
    property int actualWidth: {
        if (displayText.length === 0) {
            // 文字完全隐藏时，只占用图标宽度
            return iconSize + 4
        }
        // 有文字时，计算实际文字宽度
        var textWidth = titleText.implicitWidth
        return iconSize + textWidth + 8
    }

    property var iconGlobalPoint: {
        var a = iconContainer
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }
        return Qt.point(x, y)
    }

    Item {
        anchors.fill: parent
        id: appItem
        visible: !root.Drag.active

        AppItemPalette {
            id: itemPalette
            displayMode: root.displayMode
            colorTheme: root.colorTheme
            active: root.active
            backgroundColor: D.DTK.palette.highlight
        }
        Item {
            id: hoverBackground
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: -2
            anchors.rightMargin: 2
            width: root.actualWidth
            height: parent.height - 4
            opacity: mouseArea.containsMouse ? 1.0 : 0.0
            visible: opacity > 0
            z: -1
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                radius: 9
                color: "transparent"
                antialiasing: true
                border.color: Qt.rgba(0, 0, 0, 0.1)
                border.width: 1

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.15)
                    antialiasing: true
                    border.color: Qt.rgba(1, 1, 1, 0.1)
                    border.width: 1
                }
            }
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
        Item {
            id: iconContainer
            width: iconSize
            height: parent.height
            anchors.left: parent.left

            StatusIndicator {
                id: statusIndicator
                palette: itemPalette
                width: root.statusIndicatorSize
                height: root.statusIndicatorSize
                anchors.centerIn: icon
                visible: root.displayMode === Dock.Efficient && root.windows.length > 0 
            }

            D.DciIcon {
                id: icon
                name: root.iconName
                height: iconSize
                width: iconSize
                sourceSize: Qt.size(iconSize, iconSize)
                anchors.centerIn: parent
                retainWhileLoading: true
                scale: Panel.rootObject.isDragging ? 1.0 : 1.0

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

            Loader {
                id: aniamtionRoot
                function blendColorAlpha(fallback) {
                    var appearance = DS.applet("org.deepin.ds.dde-appearance")
                    if (!appearance || appearance.opacity < 0)
                        return fallback
                    return appearance.opacity
                }
                property real blendOpacity: blendColorAlpha(D.DTK.themeType === D.ApplicationHelper.DarkType ? 0.25 : 1.0)
                anchors.fill: icon
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
                        opacity: Math.min(3 - width / originSize, aniamtionRoot.blendOpacity)

                        SequentialAnimation {
                            running: true
                            loops: Animation.Infinite

                            ParallelAnimation {
                                NumberAnimation { target: rect; property: "width"; from: Math.max(originSize * (index - 1), 0); to: originSize * (index); duration: 1200 }
                                ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                                NumberAnimation { target: icon; property: "scale"; from: 1.0; to: 1.15; duration: 1200; easing.type: Easing.OutElastic; easing.amplitude: 1; easing.period: 0.2 }
                            }

                            ParallelAnimation {
                                NumberAnimation { target: rect; property: "width"; from: originSize * (index); to: originSize * (index + 1); duration: 1200 }
                                ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                                NumberAnimation { target: icon; property: "scale"; from: 1.15; to: 1.0; duration: 1200; easing.type: Easing.OutElastic; easing.amplitude: 1; easing.period: 0.2 }
                            }

                            ParallelAnimation {
                                NumberAnimation { target: rect; property: "width"; from: originSize * (index + 1); to: originSize * (index + 2); duration: 1200 }
                                ColorAnimation { target: rect; property: "color"; from: Qt.rgba(1, 1, 1, 0.4); to: Qt.rgba(1, 1, 1, 0.1); duration: 1200 }
                            }
                        }
                    }
                }
            }
        }

        // 标题文本，位于图标右侧
        Text {
            id: titleText
            anchors.left: iconContainer.right
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            
            text: displayText
            
            color: D.DTK.themeType === D.ApplicationHelper.DarkType ? "#FFFFFF" : "#000000"            
            font.pixelSize: textSize
            font.family: D.DTK.fontManager.t5.family
            elide: Text.ElideNone // 我们已经在displayText中处理了截断
            verticalAlignment: Text.AlignVCenter
            
            visible: displayText.length > 0
            opacity: visible ? 1.0 : 0.0
            
            
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }

        WindowIndicator {
            id: windowIndicator
            dotWidth: root.useColumnLayout  ? Math.max(iconSize / 16, 2) : Math.max(iconSize / 3, 2)
            dotHeight: root.useColumnLayout ? Math.max(iconSize / 3, 2) : Math.max(iconSize / 16, 2)
            windows: root.windows
            displayMode: root.displayMode
            useColumnLayout: root.useColumnLayout
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
                    windowIndicator.anchors.top = parent.top
                    windowIndicator.anchors.topMargin = Qt.binding(() => {return (root.height - iconSize) / 2 / 3})
                    return
                }
                case Dock.Bottom: {
                    windowIndicator.anchors.horizontalCenter = iconContainer.horizontalCenter
                    windowIndicator.anchors.bottom = parent.bottom
                    windowIndicator.anchors.bottomMargin = Qt.binding(() => {return (root.height - iconSize) / 2 / 3})
                    return
                }
                case Dock.Left: {
                    windowIndicator.anchors.verticalCenter = iconContainer.verticalCenter
                    windowIndicator.anchors.left = parent.left
                    windowIndicator.anchors.leftMargin = Qt.binding(() => {return (root.width - iconSize) / 2 / 3})
                    return
                }
                case Dock.Right:{
                    windowIndicator.anchors.verticalCenter = iconContainer.verticalCenter
                    windowIndicator.anchors.right = parent.right
                    windowIndicator.anchors.rightMargin = Qt.binding(() => {return (root.width - iconSize) / 2 / 3})
                    return
                }
                }
            }

            Component.onCompleted: {
                windowIndicator.updateIndicatorAnchors()
            }
        }

        Connections {
            function onPositionChanged() {
                windowIndicator.updateIndicatorAnchors()
                updateWindowIconGeometryTimer.start()
            }
            target: Panel
        }

        Loader {
            id: contextMenuLoader
            active: false
            property bool trashEmpty: true
            sourceComponent: LP.Menu {
                id: contextMenu
                Instantiator {
                    id: menuItemInstantiator
                    model: JSON.parse(menus)
                    delegate: LP.MenuItem {
                        text: modelData.name
                        enabled: (root.itemId === "dde-trash" && modelData.id === "clean-trash")
                                ? !contextMenuLoader.trashEmpty
                                : true
                        onTriggered: {
                            TaskManager.requestNewInstance(root.modelIndex, modelData.id);
                        }
                    }
                    onObjectAdded: (index, object) => contextMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => contextMenu.removeItem(object)
                }
            }
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
        interval: 500
        running: false
        repeat: false
        property int xOffset: 0
        property int yOffset: 0
        onTriggered: {
            if (root.windows.length != 0 || Qt.platform.pluginName === "wayland") {
                taskmanager.Applet.requestPreview(root.modelIndex, Panel.rootObject, xOffset, yOffset, Panel.position);
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
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
                iconContainer.grabToImage(function(result) {
                    root.Drag.imageSource = result.url;
                })
            }
            toolTip.close()
            closeItemPreview()
        }
        onClicked: function (mouse) {
            let index = root.modelIndex;
            if (mouse.button === Qt.RightButton) {
                contextMenuLoader.trashEmpty = TaskManager.isTrashEmpty()
                contextMenuLoader.active = true
                MenuHelper.openMenu(contextMenuLoader.item)
            } else {
                if (root.windows.length === 0) {
                    launchAnimation.start();
                    TaskManager.requestNewInstance(index, "");
                    return;
                }
                TaskManager.requestActivate(index);
            }
        }

        onEntered: {
            if (Qt.platform.pluginName === "xcb" && windows.length === 0) {
                toolTipShowTimer.start()
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
            previewTimer.xOffset = xOffset
            previewTimer.yOffset = yOffset
            previewTimer.start()
        }

        onExited: {
            if (toolTipShowTimer.running) {
                toolTipShowTimer.stop()
            }

            if (previewTimer.running) {
                previewTimer.stop()
            }

            if (Qt.platform.pluginName === "xcb" && windows.length === 0) {
                toolTip.close()
                return
            }
            closeItemPreview()
        }

        PanelToolTip {
            id: toolTip
            toolTipX: DockPanelPositioner.x
            toolTipY: DockPanelPositioner.y
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
                var point = root.mapToItem(null, root.width / 2, root.height / 2)
                toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                toolTip.text = root.itemId === "dde-trash" ? root.name + "-" + taskmanager.Applet.getTrashTipText() : root.name
                toolTip.open()
            }
        }

        function closeItemPreview() {
            if (previewTimer.running) {
                previewTimer.stop()
            } else {
                taskmanager.Applet.hideItemPreview()
            }
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["dfm_app_type_for_drag"]

        onEntered: function (drag) {
            if (root.itemId === "dde-trash") {
                var point = root.mapToItem(null, root.width / 2, root.height / 2)
                dragToolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, dragToolTip.width, dragToolTip.height)
                dragToolTip.open()
            }
        }

        onExited: function (drag) {
            if (root.itemId === "dde-trash") {
                dragToolTip.close()
            }
        }

        onDropped: function (drop){
            root.dropFilesOnItem(root.itemId, drop.urls)
        }
    }

    onWindowsChanged: {
        updateWindowIconGeometryTimer.start()
    }

    onIconGlobalPointChanged: {
        updateWindowIconGeometryTimer.start()
    }
}