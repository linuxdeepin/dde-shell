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
import org.deepin.dtk.style 1.0 as DStyle

Window {
    id: dock
    visible: Panel.hideState != Dock.Hide
    property bool useColumnLayout: Applet.position % 2
    // TODO: 临时溢出逻辑，待后面修改
    property int dockLeftSpaceForCenter: useColumnLayout ? 
        (Screen.height - dockLeftPart.implicitHeight - dockRightPart.implicitHeight) :
        (Screen.width - dockLeftPart.implicitWidth - dockRightPart.implicitWidth)
    // TODO
    signal dockCenterPartPosChanged()

    property int dockCenterPartCount: dockCenterPartModel.count

    property int dockSize: Applet.dockSize
    property int dockItemMaxSize: dockSize
    property real itemScale: 1.0
    property real draggingSize: 0.0
    property int itemIconSizeBase: 0
    property int itemSpacing: dockItemMaxSize / 20

    property bool isDragging: false

    // NOTE: -1 means not set its size, follow the platform size
    width: Panel.position == Dock.Top || Panel.position == Dock.Bottom ? -1 : dockSize
    height: Panel.position == Dock.Left || Panel.position == Dock.Right ? -1 : dockSize
    color: "transparent"
    flags: Qt.WindowDoesNotAcceptFocus

    DLayerShellWindow.anchors: position2Anchors(Applet.position)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.exclusionZone: Applet.dockSize

    D.DWindow.enabled: true
    D.DWindow.windowRadius: 0
    D.DWindow.enableBlurWindow: true

    onDockSizeChanged: {
        if (dock.dockSize === Dock.MIN_DOCK_SIZE) {
            Panel.indicatorStyle = Dock.Efficient
        } else {
            Panel.indicatorStyle = Dock.Fashion
        }
    }

    Binding on itemIconSizeBase {
        when: !isDragging
        value: dockItemMaxSize
        restoreMode: Binding.RestoreNone
    }

    // only add blendColor effect when DWindow.enableBlurWindow is true,
    // avoid to updating blur area frequently.
    D.StyledBehindWindowBlur {
        control: parent
        anchors.fill: parent
        cornerRadius: 0

        function blendColorAlpha(fallback) {
            var appearance = DS.applet("org.deepin.ds.dde-appearance")
            if (!appearance || appearance.opacity < 0)
                return fallback
            return appearance.opacity
        }
        blendColor: {
            if (valid) {
                return DStyle.Style.control.selectColor(undefined,
                                                    Qt.rgba(235 / 255.0, 235 / 255.0, 235 / 255.0, blendColorAlpha(0.6)),
                                                    Qt.rgba(0, 0, 0, blendColorAlpha(85 / 255)))
            }
            return DStyle.Style.control.selectColor(undefined,
                                                DStyle.Style.behindWindowBlur.lightNoBlurColor,
                                                DStyle.Style.behindWindowBlur.darkNoBlurColor)
        }
    }

    PropertyAnimation {
        id: hideShowAnimation;
        target: dock;
        property: useColumnLayout ? "width" : "height";
        to: Panel.hideState != Dock.Hide ? Panel.dockSize : 0;
        duration: 500
        onStarted: {
            dock.visible = true
        }
        onStopped: {
            dock.visible = (useColumnLayout ? dock.width : dock.height != 0)
        }
    }

    Timer {
        id: hideTimer
        interval: 500
        running: false
        repeat: false
        onTriggered: {
            if (!dock.isDragging)
                hideShowAnimation.start()
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
            visible: Panel.debugMode
            title: qsTr("Indicator Style")
            EnumPropertyMenuItem {
                name: qsTr("Fashion Mode")
                prop: "indicatorStyle"
                value: Dock.Fashion
            }
            EnumPropertyMenuItem {
                name: qsTr("Efficient Mode")
                prop: "indicatorStyle"
                value: Dock.Efficient
            }
        }
        MutuallyExclusiveMenu {
            title: qsTr("Alignment")
            EnumPropertyMenuItem {
                name: qsTr("Align Left")
                prop: "itemAlignment"
                value: Dock.LeftAlignment
            }
            EnumPropertyMenuItem {
                name: qsTr("Align Center")
                prop: "itemAlignment"
                value: Dock.CenterAlignment
            }
        }
        MutuallyExclusiveMenu {
            title: qsTr("Position")
            visible: Panel.debugMode
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
                visible: Panel.debugMode
                name: qsTr("Smart Hide")
                prop: "hideMode"
                value: Dock.SmartHide
            }
        }

        LP.MenuItem {
            text: qsTr("Dock Settings")
            onTriggered: {
                Panel.openDockSettings()
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
        columnSpacing: 0
        rowSpacing: 0

        Item {
            implicitWidth: 10
            implicitHeight: 10
        }

        Item {
            id: dockLeftPart
            implicitWidth: leftLoader.implicitWidth
            implicitHeight: leftLoader.implicitHeight
            OverflowContainer {
                id: leftLoader
                anchors.fill: parent
                useColumnLayout: dock.useColumnLayout
                spacing: 10
                model: DockPartAppletModel {
                    id: dockLeftPartModel
                    leftDockOrder: 0
                    rightDockOrder: 10
                }
            }
        }

        Item {
            Layout.fillWidth: Panel.itemAlignment
            Layout.fillHeight: Panel.itemAlignment
            Layout.horizontalStretchFactor: Panel.itemAlignment ? 1 : -1
        }

        Item {
            id: dockCenterPart
            implicitWidth: centerLoader.implicitWidth
            implicitHeight: centerLoader.implicitHeight
            onXChanged: dockCenterPartPosChanged()
            onYChanged: dockCenterPartPosChanged()

            OverflowContainer {
                id: centerLoader
                anchors.fill: parent
                useColumnLayout: dock.useColumnLayout
                spacing: dock.itemSpacing
                model: DockPartAppletModel {
                    id: dockCenterPartModel
                    leftDockOrder: 10
                    rightDockOrder: 20
                }
            }
            Behavior on x {
                NumberAnimation { duration: 500 }
            }
            Behavior on y {
                NumberAnimation { duration: 500 }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.horizontalStretchFactor: 1
        }

        Item {
            id: dockRightPart
            implicitWidth: rightLoader.implicitWidth
            implicitHeight: rightLoader.implicitHeight
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
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

        onPressed: function(mouse) {
            dock.isDragging = true
            oldMouseY = mouse.y
            oldMouseX = mouse.x
            draggingSize = dockSize
        }

        onPositionChanged: function(mouse) {
            var yChange = mouse.y - oldMouseY
            var xChange = mouse.x - oldMouseX
            if (Panel.position == Dock.Bottom) {
                dockSize = Math.min(Math.max(dockSize - yChange, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position == Dock.Top) {
                dockSize = Math.min(Math.max(dockSize + yChange, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position == Dock.Left) {
                dockSize = Math.min(Math.max(dockSize + xChange, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else {
                dockSize = Math.min(Math.max(dockSize - xChange, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            }
            itemScale = dockSize / draggingSize
        }

        onReleased: function(mouse) {
            dock.isDragging = false
            Applet.dockSize = dockSize
            itemScale = 1.0
            itemIconSizeBase = dockItemMaxSize
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
        function onDockSizeChanged() {
            dock.dockSize = Panel.dockSize
        }

        function onHideStateChanged() {
            if (Panel.hideState === Dock.Hide) {
                hideTimer.running = true
            } else {
                hideShowAnimation.running = true
            }
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

        DockCompositor.dockColorTheme = Qt.binding(function(){
            return Panel.colorTheme
        })
        dock.itemIconSizeBase = dock.dockItemMaxSize

        changeDragAreaAnchor()
    }
}
