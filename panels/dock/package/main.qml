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
    property bool useColumnLayout: Applet.position % 2
    // TODO: 临时溢出逻辑，待后面修改
    property int dockLeftSpaceForCenter: useColumnLayout ?
        (Screen.height - dockLeftPart.implicitHeight - dockRightPart.implicitHeight) :
        (Screen.width - dockLeftPart.implicitWidth - dockRightPart.implicitWidth)
    property int dockRemainingSpaceForCenter: useColumnLayout ?
        (Screen.height / 1.8 - dockRightPart.implicitHeight)  :
        (Screen.width / 1.8 - dockRightPart.implicitWidth) 
    property int dockPartSpacing: gridLayout.columnSpacing
    // TODO
    signal dockCenterPartPosChanged()
    signal pressedAndDragging(bool isDragging)
    signal viewDeactivated()

    property int dockCenterPartCount: dockCenterPartModel.count

    property int dockSize: Applet.dockSize
    property int dockItemMaxSize: dockSize
    property int itemIconSizeBase: 0
    property int itemSpacing: 0

    property bool isDragging: false

    property real dockItemIconSize: dockItemMaxSize * 9 / 14

    // NOTE: -1 means not set its size, follow the platform size
    width: Panel.position === Dock.Top || Panel.position === Dock.Bottom ? -1 : dockSize
    height: Panel.position === Dock.Left || Panel.position === Dock.Right ? -1 : dockSize
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
    DLayerShellWindow.scope: "dde-shell/dock"
    DLayerShellWindow.keyboardInteractivity: DLayerShellWindow.KeyboardInteractivityOnDemand

    D.DWindow.enabled: true
    D.DWindow.windowRadius: 0
    //TODO：由于windoweffect处理有BUG，导致动画结束后一致保持无阴影，无borderwidth状态。 无法恢复到最初的阴影和边框
    //D.DWindow.windowEffect: hideShowAnimation.running ? D.PlatformHandle.EffectNoShadow | D.PlatformHandle.EffectNoBorder : 0
    
    // 目前直接处理shadowColor(透明和默认值的切换)和borderWidth(0和1的切换)，来控制阴影和边框
    // 参数默认值见： https://github.com/linuxdeepin/qt5platform-plugins/blob/master/xcb/dframewindow.h#L122
    // 需要注意，shadowRadius不能直接套用于“扩散”参数，拿到不透明度100%的设计图确定radius更合适一些。
    D.DWindow.shadowColor: hideShowAnimation.running ? Qt.rgba(0, 0, 0, 0) : Qt.rgba(0, 0, 0, 0.1)
    D.DWindow.shadowOffset: Qt.point(0, 0)
    D.DWindow.shadowRadius: 40
    D.DWindow.borderWidth:  hideShowAnimation.running ? 0 : 1
    D.DWindow.enableBlurWindow: Qt.platform.pluginName !== "xcb"
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

    PropertyAnimation {
        id: hideShowAnimation;
        // Currently, Wayland (Treeland) doesn't support StyledBehindWindowBlur inside the window, thus we keep using the window size approach on Wayland
        property bool useTransformBasedAnimation: Qt.platform.pluginName === "xcb"
        target: useTransformBasedAnimation ? dockTransform : dock;
        property: {
            if (useTransformBasedAnimation) return dock.useColumnLayout ? "x" : "y";
            return dock.useColumnLayout ? "width" : "height";
        }
        to: {
            if (useTransformBasedAnimation) return Panel.hideState !== Dock.Hide ? 0 : ((Panel.position === Dock.Left || Panel.position === Dock.Top) ? -Panel.dockSize : Panel.dockSize);
            return Panel.hideState !== Dock.Hide ? Panel.dockSize : 1;
        }
        duration: 500
        easing.type: Easing.OutCubic
        onStarted: {
            dock.visible = true
        }
        onStopped: {
            if (useTransformBasedAnimation) {
                dock.visible = ((dock.useColumnLayout ? dockTransform.x : dockTransform.y) === 0)
            } else {
                dock.visible = ((dock.useColumnLayout ? dock.width : dock.height) !== 1)
            }
        }
    }

    Connections {
        target: dockTransform
        enabled: Qt.platform.pluginName === "xcb" && hideShowAnimation.running
        
        function onXChanged() {
            if (dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(dockTransform.x, 0)
            }
        }
        
        function onYChanged() {
            if (!dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(0, dockTransform.y)
            }
        }
    }

    Connections {
        target: dock
        enabled: Qt.platform.pluginName !== "xcb" && hideShowAnimation.running
        
        function onWidthChanged() {
            if (dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(dock.width, 0)
            }
        }
        
        function onHeightChanged() {
            if (!dock.useColumnLayout) {
                Panel.notifyDockPositionChanged(0, dock.height)
            }
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

    SequentialAnimation {
        id: dockAnimation
        property bool useTransformBasedAnimation: Qt.platform.pluginName === "xcb"
        property bool isShowing: false
        property var target: useTransformBasedAnimation ? dockTransform : dock
        property string animProperty: {
            if (useTransformBasedAnimation) return dock.useColumnLayout ? "x" : "y";
            return dock.useColumnLayout ? "width" : "height";
        }

        function startAnimation(showing) {
            isShowing = showing;
            start();
        }

        PropertyAnimation {
            target: dockAnimation.target
            property: dockAnimation.animProperty
            from: {
                if (dockAnimation.isShowing) {
                    if (dockAnimation.useTransformBasedAnimation) {
                        return (Panel.position === Dock.Left || Panel.position === Dock.Top) ? -Panel.dockSize : Panel.dockSize;
                    }
                    return 1;
                }
                return 0;
            }
            to: {
                if (dockAnimation.isShowing) {
                    return 0;
                } else {
                    if (dockAnimation.useTransformBasedAnimation) {
                        return (Panel.position === Dock.Left || Panel.position === Dock.Top) ? -Panel.dockSize : Panel.dockSize;
                    }
                    return 1;
                }
            }
            duration: 250
            easing.type: Easing.OutCubic
        }

        onStarted: {
            dock.visible = true;
        }

        onStopped: {
            if (useTransformBasedAnimation) {
                dock.visible = true;
            } else {
                dock.visible = ((dock.useColumnLayout ? dock.width : dock.height) !== 1);
            }
        }
    }

    component EnumPropertyMenuItem: LP.MenuItem {
        required property string name
        required property string prop
        required property int value
        text: name

        property var positionChangeCallback: function() {
            // Disconnect any existing callback first
            dockAnimation.onStopped.disconnect(positionChangeCallback);
            // Stop any running animations first --fix bug with do not show dock
            dockAnimation.stop();
            // Reset transform before starting new animation--fix bug with change position,will have a blank area
            dockTransform.x = 0;
            dockTransform.y = 0;

            Applet[prop] = value;
            checked = Qt.binding(function() {
                return Applet[prop] === value;
            });
            dockAnimation.startAnimation(true);
        }
        onTriggered: {
            if (prop === "position") {
                // Connect the callback and start the hide animation
                dockAnimation.onStopped.connect(positionChangeCallback);
                dockAnimation.startAnimation(false);
            } else {
                Applet[prop] = value
                checked = Qt.binding(function() {
                    return Applet[prop] === value
                })
            }
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
                title: qsTr("Mode")
                EnumPropertyMenuItem {
                    name: qsTr("Classic Mode")
                    prop: "itemAlignment"
                    value: Dock.LeftAlignment
                }
                EnumPropertyMenuItem {
                    name: qsTr("Centered Mode")
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
                    name: qsTr("Smart Hide")
                    prop: "hideMode"
                    value: Dock.SmartHide
                }
            }
            LP.MenuItem {
                text: qsTr("Lock the Dock")
                checked: Panel.locked
                onTriggered: {
                    Panel.locked = !Panel.locked
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

    Item {
        id: dockContainer
        width: dock.useColumnLayout ? dock.dockSize : parent.width
        height: dock.useColumnLayout ? parent.height : dock.dockSize
        anchors {
            left: parent.left
            top: parent.top
        }
        transform: Translate {
            id: dockTransform
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

        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            gesturePolicy: TapHandler.WithinBounds
            onTapped: function(eventPoint, button) {
                let lastActive = MenuHelper.activeMenu
                MenuHelper.closeCurrent()
                dockMenuLoader.active = true
                if (button === Qt.RightButton && lastActive !== dockMenuLoader.item) {
                    // maybe has popup visible, close it.
                    Panel.requestClosePopup()
                    viewDeactivated()
                    MenuHelper.openMenu(dockMenuLoader.item)
                }
                if (button === Qt.LeftButton) {
                    // try to close popup when clicked empty, because dock does not have focus.
                    Panel.requestClosePopup()
                    viewDeactivated()
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

        // TODO: remove GridLayout and use delegatechosser manager all items
        GridLayout {
            id: gridLayout
            anchors.fill: parent
            columns: 1
            rows: 1
            flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
            property real itemMargin: Math.max((dockItemIconSize / 48 * 10))
            columnSpacing: dockLeftPartModel.count > 0 ? 10 : itemMargin
            rowSpacing: columnSpacing

            Item {
                id: leftMargin
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
                    model: DockPartAppletModel {
                        id: dockLeftPartModel
                        leftDockOrder: 0
                        rightDockOrder: 10
                    }
                }
            }

            Item {  
                id: dockCenterPart
                property var taskmanagerRootObject: {
                    let applet = DS.applet("org.deepin.ds.dock.taskmanager")
                    return applet ? applet.rootObject : null
                }
                
                readonly property real taskmanagerImplicitWidth: taskmanagerRootObject ? taskmanagerRootObject.implicitWidth : 0
                readonly property real taskmanagerImplicitHeight: taskmanagerRootObject ? taskmanagerRootObject.implicitHeight : 0
                readonly property real taskmanagerAppContainerWidth: taskmanagerRootObject ? taskmanagerRootObject.appContainerWidth : 0
                readonly property real taskmanagerAppContainerHeight: taskmanagerRootObject ? taskmanagerRootObject.appContainerHeight : 0
                
                implicitWidth: centerLoader.implicitWidth - taskmanagerImplicitWidth + taskmanagerAppContainerWidth
                implicitHeight: centerLoader.implicitHeight - taskmanagerImplicitHeight + taskmanagerAppContainerHeight
                onXChanged: dockCenterPartPosChanged()
                onYChanged: dockCenterPartPosChanged()
                Layout.leftMargin: !useColumnLayout && Panel.itemAlignment === Dock.CenterAlignment ?
                    (dock.width - dockCenterPart.implicitWidth) / 2 - (dockLeftPart.implicitWidth + 20) + Math.min((dock.width - dockCenterPart.implicitWidth) / 2 - (dockRightPart.implicitWidth + 20), 0) : 0
                Layout.topMargin: useColumnLayout && Panel.itemAlignment === Dock.CenterAlignment ?
                    (dock.height - dockCenterPart.implicitHeight) / 2 - (dockLeftPart.implicitHeight + 20) + Math.min((dock.height - dockCenterPart.implicitHeight) / 2 - (dockRightPart.implicitHeight + 20), 0) : 0

                Behavior on Layout.leftMargin {
                    enabled: !dock.isDragging
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }

                Behavior on Layout.topMargin {
                    enabled: !dock.isDragging
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                }

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
        }

        Item {
            id: dockRightPart
            implicitWidth: rightLoader.implicitWidth
            implicitHeight: rightLoader.implicitHeight
            anchors.right: parent.right
            anchors.bottom: parent.bottom
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
        enabled: !Panel.locked

        cursorShape: {
            if (Panel.locked) {
                return Qt.ArrowCursor
            }
            if (Panel.position === Dock.Top || Panel.position === Dock.Bottom) {
                return Qt.SizeVerCursor
            }
            return Qt.SizeHorCursor
        }

        onPressed: function(mouse) {
            if (Panel.locked) return
            dock.isDragging = true
            oldMousePos = mapToGlobal(mouse.x, mouse.y)
            oldDockSize = dockSize
            recentDeltas = []
            Panel.requestClosePopup()
            DS.grabMouse(Panel.rootObject, true)
        }

        // this used for blocking MouseEvent sent to bottom MouseArea
        onClicked: {}

        onPositionChanged: function(mouse) {
            if (Panel.locked || !dock.isDragging) return
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
            if (Panel.position === Dock.Bottom) {
                newDockSize = Math.min(Math.max(oldDockSize - changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position === Dock.Top) {
                newDockSize = Math.min(Math.max(oldDockSize + changeAverage, Dock.MIN_DOCK_SIZE), Dock.MAX_DOCK_SIZE)
            } else if (Panel.position === Dock.Left) {
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
            if (Panel.locked) return
            dock.isDragging = false
            Applet.dockSize = dockSize
            itemIconSizeBase = dockItemMaxSize
            pressedAndDragging(false)
            DS.grabMouse(Panel.rootObject, false)
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
            Panel.requestClosePopup()
        }
        function onDockSizeChanged() {
            dock.dockSize = Panel.dockSize
        }

        function onHideStateChanged() {
            if (Panel.hideState === Dock.Hide) {
                hideTimer.running = true
            } else {
                hideShowAnimation.restart()
            }
        }
        function onRequestClosePopup() {
            let popup = Panel.popupWindow
            DS.closeChildrenWindows(popup)
            if (popup && popup.visible)
                popup.close()
        }
        function onDockScreenChanged() {
            // Close all popups when dock moves to another screen
            Panel.requestClosePopup()
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

        DockCompositor.dockSize = Qt.binding(function(){
            return Qt.size(Panel.frontendWindowRect.width, Panel.frontendWindowRect.height)
        })

        DockCompositor.panelScale = Qt.binding(function(){
            return Panel.devicePixelRatio
        })

        dock.itemIconSizeBase = dock.dockItemMaxSize
        dock.visible = Panel.hideState !== Dock.Hide
        changeDragAreaAnchor()
    }
}
