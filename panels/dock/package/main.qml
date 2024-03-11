// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 2.15
import QtQuick.Window 2.15

import QtQml
import Qt.labs.platform as LP

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

Window {
    id: dock
    visible: true
    property bool useColumnLayout: Applet.position % 2

    // NOTE: -1 means not set its size, follow the platform size
    width: Panel.position == Dock.Top || Panel.position == Dock.Bottom ? -1 : Applet.dockSize
    height: Panel.position == Dock.Left || Panel.position == Dock.Right ? -1 : Applet.dockSize
    color: "transparent"

    DLayerShellWindow.anchors: position2Anchors(Applet.position)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.exclusionZone: Applet.dockSize
    DLayerShellWindow.leftMargin: (useColumnLayout || Applet.displayMode === Dock.Efficient) ? 0 : (Screen.width - gridLayout.implicitWidth) / 2
    DLayerShellWindow.rightMargin: (useColumnLayout || Applet.displayMode === Dock.Efficient) ? 0 : (Screen.width - gridLayout.implicitWidth) / 2
    DLayerShellWindow.topMargin: (!useColumnLayout || Applet.displayMode === Dock.Efficient) ? 0 : (Screen.height - gridLayout.implicitHeight) / 2
    DLayerShellWindow.bottomMargin: (!useColumnLayout || Applet.displayMode === Dock.Efficient) ? 0 : (Screen.height - gridLayout.implicitHeight) / 2

    D.DWindow.enabled: true
    D.DWindow.windowRadius: 0

    // TODO: wait BehindWindowblur support setting radius for special corners

    D.RoundRectangle {
        anchors.fill: parent
        radius: (Applet.dockSize - 20) / 2
        color: Applet.colorTheme == Dock.Light ? "ivory" : "darkgrey"
        corners: {
            if (Panel.displayMode == Dock.Efficient)
                return D.RoundRectangle.NoneCorner

            switch (Panel.position) {
            case Dock.Top:
                return D.RoundRectangle.BottomCorner
            case Dock.Right:
                return D.RoundRectangle.LeftCorner
            case Dock.Bottom:
                return D.RoundRectangle.TopCorner
            case Dock.Left:
                return D.RoundRectangle.RightCorner
            }
        }
    }

    component EnumPropertyMenuItem: LP.MenuItem {
        required property string name
        required property string prop
        required property int value
        text: name
        onTriggered: {
            Applet[prop] = value
            checked = true
        }
        checked: Applet[prop] === value
    }
    component MutuallyExclusiveMenu: LP.Menu {
        id: menu
        LP.MenuItemGroup {
            id: group
            items: menu.items
        }
    }

    function updateAppItems()
    {
        dockLeftPartModel.update()
        dockCenterPartModel.update()
        dockRightPartModel.update()
    }

    LP.Menu {
        id: dockMenu
        MutuallyExclusiveMenu {
            title: qsTr("Mode")
            EnumPropertyMenuItem {
                name: qsTr("Fashion Mode")
                prop: "displayMode"
                value: Dock.Fashion
            }
            EnumPropertyMenuItem {
                name: qsTr("Efficient Mode")
                prop: "displayMode"
                value: Dock.Efficient
            }
        }
        MutuallyExclusiveMenu {
            title: qsTr("Position")
            EnumPropertyMenuItem {
                enabled: Panel.debugMode
                name: qsTr("Top")
                prop: "position"
                value: Dock.Top
            }
            EnumPropertyMenuItem {
                name: qsTr("Bottom")
                prop: "position"
                value: Dock.Bottom
            }
            EnumPropertyMenuItem {
                enabled: Panel.debugMode
                name: qsTr("Left")
                prop: "position"
                value: Dock.Left
            }
            EnumPropertyMenuItem {
                enabled: Panel.debugMode
                name: qsTr("Right")
                prop: "position"
                value: Dock.Right
            }
        }
        MutuallyExclusiveMenu {
            enabled: Panel.debugMode
            title: qsTr("Status")
            EnumPropertyMenuItem {
                name: qsTr("Keep Shown")
                prop: "hideMode"
                value: Dock.KeepShowing
            }
            EnumPropertyMenuItem {
                name: qsTr("Keep Hidden")
                prop: "hideMode"
                value: Dock.KeepHidden
            }
            EnumPropertyMenuItem {
                name: qsTr("Smart Hide")
                prop: "hideMode"
                value: Dock.SmartHide
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        gesturePolicy: TapHandler.WithinBounds
        onTapped: function(eventPoint, button) {
            let lastActive = MenuHelper.activeMenu
            MenuHelper.closeCurrent()
            if (button === Qt.RightButton && lastActive !== dockMenu) {
                MenuHelper.openMenu(dockMenu)
            }
        }
    }

    // TODO missing property binding to update ProxyModel's filter and sort opearation.
    Repeater {
        model: Applet.appletItems
        delegate: Item {
            property var order: model.data.dockOrder
            property bool itemVisible: model.data.shouldVisible

            onItemVisibleChanged: {
                updateAppItems()
            }
            onOrderChanged: {
                updateAppItems()
            }
        }
    }

    GridLayout {
        id: gridLayout
        anchors.fill: parent
        columns: 1
        rows: 1
        flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom

        Item {
            id: dockLeftPart
            implicitWidth: leftLoader.implicitWidth
            implicitHeight: leftLoader.implicitHeight
            OverflowContainer {
                id: leftLoader
                anchors.fill: parent
                useColumnLayout: dock.useColumnLayout
                model: DockPartAppletModel {
                    id: dockLeftPartModel
                    leftDockOrder: 0
                    rightDockOrder: 10
                }
            }
        }

        Rectangle {
            visible: (Applet.displayMode === Dock.Fashion && leftLoader.model.count !== 0)
            implicitWidth: Applet.dockSize * 0.6 + (dock.useColumnLayout ? 0 : 22)
            implicitHeight: Applet.dockSize * 0.6 + (dock.useColumnLayout ? 22 : 0)
            color: "transparent"
        }

        Item {
            id: dockCenterPart
            implicitWidth: centerLoader.implicitWidth
            implicitHeight: centerLoader.implicitHeight
            Layout.fillWidth: true
            Layout.fillHeight: true
            OverflowContainer {
                id: centerLoader
                anchors.fill: parent
                useColumnLayout: dock.useColumnLayout
                model: DockPartAppletModel {
                    id: dockCenterPartModel
                    leftDockOrder: 10
                    rightDockOrder: 20
                }
            }
        }

        Rectangle {
            visible: (Applet.displayMode === Dock.Fashion && rightLoader.model.count !== 0)
            implicitWidth: Applet.dockSize * 0.6 + (dock.useColumnLayout ? 0 : 22)
            implicitHeight: Applet.dockSize * 0.6 + (dock.useColumnLayout ? 22 : 0)
            color: "transparent"
        }

        Item {
            id: dockRightPart
            implicitWidth: rightLoader.implicitWidth
            implicitHeight: rightLoader.implicitHeight
            OverflowContainer {
                id: rightLoader
                anchors.fill: parent
                useColumnLayout: dock.useColumnLayout
                model: DockPartAppletModel {
                    id: dockRightPartModel
                    leftDockOrder: 20
                    rightDockOrder: 30
                }
            }
        }
    }
    MouseArea {
        id: dragArea
        property int oldMouseY: 0;
        property int oldMouseX: 0;
  
        cursorShape: {
            if (Panel.position == Dock.Top || Panel.position == Dock.Bottom) {
                return Qt.SizeVerCursor
            }
            return Qt.SizeHorCursor
        }

        onPressed:function(mouse) {
            oldMouseY = mouse.y
            oldMouseX = mouse.x
        }
        onPositionChanged:function(mouse) {
            var yChange = mouse.y - oldMouseY
            var xChange = mouse.x - oldMouseX
            if (Panel.position == Dock.Bottom) {
                Applet.dockSize -= yChange
            } else if (Panel.position == Dock.Top) {
                Applet.dockSize += yChange
            } else if (Panel.position == Dock.Left) {
                Applet.dockSize += xChange
            } else {
                Applet.dockSize -= xChange
            }
        }
        onReleased:function(mouse) {
            var yChange = mouse.y - oldMouseY
            var xChange = mouse.x - oldMouseX
            if (Panel.position == Dock.Bottom) {
                Applet.dockSize -= yChange
            } else if (Panel.position == Dock.Top) {
                Applet.dockSize += yChange
            } else if (Panel.position == Dock.Left) {
                Applet.dockSize += xChange
            } else {
                Applet.dockSize -= xChange
            }
        }

        function anchorToTop() {
            anchors.top = undefined
            anchors.bottom = parent.bottom
            anchors.left = parent.left
            anchors.right = parent.right
            dragArea.height = 5
        }
        function anchorToBottom() {
            anchors.bottom = undefined
            anchors.top = parent.top
            anchors.left = parent.left
            anchors.right = parent.right
            dragArea.height = 5
        }
        function anchorToLeft() {
            anchors.left = undefined
            anchors.right = parent.right
            anchors.bottom = parent.bottom
            anchors.top = parent.top
            dragArea.width = 5
        }
        function anchorToRight() {
            anchors.right = undefined
            anchors.left = parent.left
            anchors.bottom = parent.bottom
            anchors.top = parent.top
            dragArea.width = 5
        }

    }

    function changeDragAreaAnchor() {
        switch(Panel.position) {
        case Dock.Top: {
            dragArea.anchorToTop()
            return
        }
        case Dock.Bottom: {
            dragArea.anchorToBottom()
            return
        }
        case Dock.Left: {
            dragArea.anchorToLeft()
            return
        }
        case Dock.Right:{
            dragArea.anchorToRight()
            return
        }
        }
    }

    Connections {
        function onPositionChanged() {
            changeDragAreaAnchor()
        }
        target: Panel
    }

    function position2Anchors(position) {
        switch (position) {
        case Dock.Top:
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorTop
        case Dock.Right:
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorRight
        case Dock.Bottom:
            return DLayerShellWindow.AnchorLeft | DLayerShellWindow.AnchorRight | DLayerShellWindow.AnchorBottom
        case Dock.Left:
            return DLayerShellWindow.AnchorTop | DLayerShellWindow.AnchorBottom | DLayerShellWindow.AnchorLeft
        }
    }

    // can not move into DockCompositor
    // Panel get a object instead of ds::dock::DockPanel during DockCompositor creating
    Binding {
        target: Panel
        property: "compositorReady"
        value: DockCompositor.compositor.created
        when: DockCompositor.compositor.created
    }

    Component.onCompleted: {
        DockCompositor.dockPosition = Qt.binding(function() {
            return Panel.position
        })

        DockCompositor.dockDisplayMode = Qt.binding(function(){
            return Panel.displayMode
        })

        DockCompositor.dockColorTheme = Qt.binding(function(){
            return Panel.colorTheme
        })

        changeDragAreaAnchor()
    }
}
