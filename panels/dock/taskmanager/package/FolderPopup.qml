// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

// Source-neutral container: files and applications provide data and callbacks,
// never a second implementation of the popup's sizing, layout or lifecycle.
PanelPopup {
    id: root
    property string folderName
    property var contents: []
    property var presentation: null
    property point anchorPoint: Qt.point(0, 0)
    property bool interactionActive: false
    signal activated(var entry)
    signal contextMenuRequested(var entry)

    readonly property bool hasProviderContent: presentation !== null && presentation !== undefined
    readonly property int itemCount: contents.length
    readonly property int columns: itemCount <= 8 ? 4 : 5
    readonly property int rows: Math.max(1, Math.ceil(itemCount / columns))
    readonly property real cellWidth: 96
    readonly property real cellHeight: 104
    readonly property var screen: Panel.rootObject.screen
    // desktopAvailableWidth/Height describe the virtual desktop, not the
    // current monitor. Use its work area, including offsets from other panels.
    readonly property rect workArea: DockPanelPositioner.availableGeometry
    readonly property real screenWidth: workArea.width > 0 ? workArea.width : (screen ? screen.width : 1920)
    readonly property real screenHeight: workArea.height > 0 ? workArea.height : (screen ? screen.height : 1080)
    readonly property real screenX: workArea.width > 0 ? workArea.x : (screen ? screen.virtualX : 0)
    readonly property real screenY: workArea.height > 0 ? workArea.y : (screen ? screen.virtualY : 0)
    readonly property bool verticalDock: Panel.position === Dock.Left || Panel.position === Dock.Right
    // Room for a full row of four cells, so a sparse folder still reads as a
    // popup instead of a narrow strip.
    readonly property real minimumWidth: 420
    readonly property real maximumWidth: screenWidth - (verticalDock ? Panel.rootObject.width + 30 : 20)
    readonly property real maximumHeight: screenHeight - (verticalDock ? 20 : Panel.rootObject.height + 30)
    width: hasProviderContent
            ? Math.max(1, Math.min(maximumWidth, contentLoader.item ? contentLoader.item.implicitWidth : 320))
            : Math.min(Math.max(minimumWidth, columns * cellWidth + 24), maximumWidth)
    height: hasProviderContent
            ? Math.max(1, Math.min(maximumHeight, contentLoader.item ? contentLoader.item.implicitHeight : 120))
            : Math.min(Math.min(3, rows) * cellHeight + 56, maximumHeight)
    contentReady: hasProviderContent ? contentLoader.status !== Loader.Loading : true
    popupX: Math.max(screenX - Panel.rootObject.x + 10,
                     Math.min(DockPanelPositioner.x, screenX + screenWidth - Panel.rootObject.x - width - 10))
    popupY: Math.max(screenY - Panel.rootObject.y + 10,
                     Math.min(DockPanelPositioner.y, screenY + screenHeight - Panel.rootObject.y - height - 10))
    closeOnInactive: !interactionActive && !Panel.contextDragging
    DockPanelPositioner.bounding: Qt.rect(anchorPoint.x, anchorPoint.y, width, height)

    function openAt(point) {
        anchorPoint = point
        flick.contentY = 0
        open()
    }

    onPresentationChanged: {
        const url = presentation ? presentation.contentUrl : ""
        contentLoader.setSource(url, url ? {presentation: presentation} : {})
    }

    Item {
        anchors.fill: parent
        Loader {
            id: contentLoader
            objectName: "providerContentLoader"
            anchors.fill: parent
            active: root.hasProviderContent
        }
        Label {
            anchors.fill: parent
            visible: root.hasProviderContent && (contentLoader.status === Loader.Error || contentLoader.status === Loader.Null)
            text: qsTr("Content unavailable")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Connections {
            target: contentLoader.item
            ignoreUnknownSignals: true
            function onCloseRequested() { root.close() }
        }
        Item {
            anchors.fill: parent
            visible: !root.hasProviderContent
            Text {
                anchors.top: parent.top
                anchors.topMargin: 12
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 32
                height: 28
                text: root.folderName
                textFormat: Text.PlainText
                color: D.DTK.palette.windowText
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                font: D.DTK.fontManager.t6
            }
            Flickable {
                id: flick
                anchors.fill: parent
                anchors.topMargin: 44
                anchors.bottomMargin: 12
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                contentWidth: width
                contentHeight: root.rows * root.cellHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Grid {
                    columns: root.columns
                    Repeater {
                        model: root.contents
                        ItemDelegate {
                            id: entryDelegate
                            required property var modelData
                            width: flick.width / root.columns
                            height: root.cellHeight
                            Accessible.name: modelData.name
                            onClicked: root.activated(modelData)
                            background: Rectangle {
                                radius: 8
                                color: D.DTK.palette.highlight
                                opacity: entryDelegate.down ? 0.25 : entryDelegate.hovered ? 0.12 : 0
                            }
                            contentItem: Item {
                                D.DciIcon {
                                    anchors.top: parent.top
                                    anchors.topMargin: 4
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 48
                                    height: 48
                                    sourceSize: Qt.size(width, height)
                                    name: entryDelegate.modelData.iconName
                                }
                                Text {
                                    anchors.bottom: parent.bottom
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width
                                    height: 36
                                    text: entryDelegate.modelData.name
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                    color: D.DTK.palette.windowText
                                    font: D.DTK.fontManager.t8
                                }
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: root.contextMenuRequested(entryDelegate.modelData)
                            }
                        }
                    }
                }
            }
        }
    }
}
