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
    signal pressedAndDragging(bool isDragging)

    property int dockCenterPartCount: dockCenterPartModel.count

    property int dockSize: Applet.dockSize
    property int dockItemMaxSize: dockSize
    property int itemIconSizeBase: 0
    property int itemSpacing: 0

    property bool isDragging: false

    // NOTE: -1 means not set its size, follow the platform size
    width: Panel.position == Dock.Top || Panel.position == Dock.Bottom ? -1 : dockSize
    height: Panel.position == Dock.Left || Panel.position == Dock.Right ? -1 : dockSize
    color: "transparent"
    flags: Qt.WindowDoesNotAcceptFocus

    function blendColorAlpha(fallback) {
        var appearance = DS.applet("org.deepin.ds.dde-appearance")
        if (!appearance || appearance.opacity < 0)
            return fallback
        return appearance.opacity
    }

    DLayerShellWindow.anchors: position2Anchors(Applet.position)
    DLayerShellWindow.layer: DLayerShellWindow.LayerTop
    DLayerShellWindow.exclusionZone: Panel.hideMode === Dock.KeepShowing ? Applet.dockSize : 0

    D.DWindow.enabled: true
    D.DWindow.windowRadius: 0
    D.DWindow.enableBlurWindow: true
    D.DWindow.themeType: Panel.colorTheme
    D.DWindow.borderColor: D.DTK.themeType === D.ApplicationHelper.DarkType ? Qt.rgba(0, 0, 0, dock.blendColorAlpha(0.6) + 20 / 255) : Qt.rgba(0, 0, 0, 0.15)
    D.ColorSelector.family: D.Palette.CrystalColor

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
    // avoid to updating blur area frequently.--
    D.StyledBehindWindowBlur {
        control: parent
        anchors.fill: parent
        cornerRadius: 0
        blendColor: {
            if (valid) {
                return DStyle.Style.control.selectColor(undefined,
                                                    Qt.rgba(235 / 255.0, 235 / 255.0, 235 / 255.0, dock.blendColorAlpha(0.6)),
                                                    Qt.rgba(10 / 255, 10 / 255, 10 /255, dock.blendColorAlpha(85 / 255)))
            }
            return DStyle.Style.control.selectColor(undefined,
                                                DStyle.Style.behindWindowBlur.lightNoBlurColor,
                                                DStyle.Style.behindWindowBlur.darkNoBlurColor)
        }
    }

    D.InsideBoxBorder {
        anchors.fill: parent
        // TODO 小数倍缩放下可能会导致边框时有时无，设置0.5的margin规避，不过可能会导致内外边框之间多1px的填充，不是完美的解决方案
        anchors.margins: 0.5
        color: D.DTK.themeType === D.ApplicationHelper.DarkType ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, dock.blendColorAlpha(0.6) - 0.05)
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
            checked = Qt.binding(function() {
                return Applet[prop] === value
            })
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

    Loader {
        id: dockMenuLoader
        active: false
        sourceComponent: LP.Menu {
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
                EnumPropertyMenuItem {
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
                    name: qsTr("Left")
                    prop: "position"
                    value: Dock.Left
                }
                EnumPropertyMenuItem {
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
                    enabled: Panel.debugMode
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
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        gesturePolicy: TapHandler.WithinBounds
        onTapped: function(eventPoint, button) {
            let lastActive = MenuHelper.activeMenu
            MenuHelper.closeCurrent()
            dockMenuLoader.active = true
            if (button === Qt.RightButton && lastActive !== dockMenuLoader.item) {
                MenuHelper.openMenu(dockMenuLoader.item)
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.ArrowCursor
    }

    // TODO missing property binding to update ProxyModel's filter and sort opearation.
    Repeater {
        model: Applet.appletItems
        delegate: Item {
            property var order: model.data.dockOrder
            property bool itemVisible: model.data.shouldVisible === undefined || model.data.shouldVisible

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
        columnSpacing: 10
        rowSpacing: 10

        Item {
            id: leftMargin
            visible: dockLeftPartModel.count > 0
            implicitWidth: 0
            implicitHeight: 0
        }

        Item {
            id: dockLeftPart
            visible: dockLeftPartModel.count > 0
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
            id: dockCenterPart
            implicitWidth: centerLoader.implicitWidth
            implicitHeight: centerLoader.implicitHeight
            onXChanged: dockCenterPartPosChanged()
            onYChanged: dockCenterPartPosChanged()
            Layout.leftMargin: !useColumnLayout && Panel.itemAlignment === Dock.CenterAlignment ?
                (dock.width - dockCenterPart.implicitWidth) / 2 - (dockLeftPart.implicitWidth + 20) + Math.min((dock.width - dockCenterPart.implicitWidth) / 2 - (dockRightPart.implicitWidth + 20), 0) : 0
            Layout.topMargin: useColumnLayout && Panel.itemAlignment === Dock.CenterAlignment ?
                (dock.height - dockCenterPart.implicitHeight) / 2 - (dockLeftPart.implicitHeight + 20) + Math.min((dock.height - dockCenterPart.implicitHeight) / 2 - (dockRightPart.implicitHeight + 20), 0) : 0

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
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
        property point oldMousePos: Qt.point(0, 0)
        property int oldDockSize: 0
        property list<int> recentDeltas: []
        property int averageCount: 5
        hoverEnabled: true
        propagateComposedEvents: true

        cursorShape: {
            if (Panel.position == Dock.Top || Panel.position == Dock.Bottom) {
                return Qt.SizeVerCursor
            }
            return Qt.SizeHorCursor
        }

        onPressed: function(mouse) {
            var launcherItem = DS.applet("org.deepin.ds.launchpad")
            if (launcherItem) {
                launcherItem.rootObject.hide()
            }
            dock.isDragging = true
            oldMousePos = mapToGlobal(mouse.x, mouse.y)
            oldDockSize = dockSize
            recentDeltas = []
            Panel.setMouseGrabEnabled(dragArea, true);
        }

        // this used for blocking MouseEvent sent to bottom MouseArea
        onClicked: {}

        onPositionChanged: function(mouse) {
            if (!dock.isDragging) return
            var newPos = mapToGlobal(mouse.x, mouse.y)
            var xChange = newPos.x - oldMousePos.x
            var yChange = newPos.y - oldMousePos.y

            if (Panel.position === Dock.Bottom || Panel.position === Dock.Top) {
                recentDeltas.push(yChange)
            } else {
                recentDeltas.push(xChange)
            }

            if (recentDeltas.length > averageCount) {
                recentDeltas.shift()
            }
            // Taking the average makes the data smooth without jumps
            var changeAverage = recentDeltas.reduce(function(acc, val) { return acc + val; }, 0) / recentDeltas.length;

            var newDockSize = 0
            if (Panel.position == Dock.Bottom) {
                newDockSize = Math.min(Math.max(oldDockSize - changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position == Dock.Top) {
                newDockSize = Math.min(Math.max(oldDockSize + changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position == Dock.Left) {
                newDockSize = Math.min(Math.max(oldDockSize + changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else {
                newDockSize = Math.min(Math.max(oldDockSize - changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            }

            if (newDockSize !== dockSize) {
                dockSize = newDockSize
            }

            pressedAndDragging(true)
        }

        onReleased: function(mouse) {
            dock.isDragging = false
            Applet.dockSize = dockSize
            itemIconSizeBase = dockItemMaxSize
            pressedAndDragging(false)
            Panel.setMouseGrabEnabled(dragArea, false);
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
        Panel.toolTipWindow.D.DWindow.themeType = Qt.binding(function(){
            return Panel.colorTheme
        })

        Panel.popupWindow.D.DWindow.themeType = Qt.binding(function(){
            return Panel.colorTheme
        })

        DockCompositor.dockColorTheme = Qt.binding(function(){
            return Panel.colorTheme
        })

        DockCompositor.dockPosition = Qt.binding(function(){
            return Panel.position
        })

        dock.itemIconSizeBase = dock.dockItemMaxSize

        changeDragAreaAnchor()
    }
}
