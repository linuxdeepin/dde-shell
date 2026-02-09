// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: control
    implicitWidth: 430
    implicitHeight: listview.height + listview.anchors.topMargin + listview.anchors.bottomMargin

    readonly property bool singleView: false
    property int selectIndex: indexByValue(Applet.effectType)
    property int checkedIndex: indexByValue(Applet.effectType)

    enum WindowEffectType {
        Default = 0,
        Best,
        Better,
        Good,
        Normal,
        Compatible
    }

    Connections {
        target: control.Panel
        enabled: match(control.Panel.osdType)
        function onVisibleChanged() {
            if (!control.Panel.visible) {
                Applet.effectType = effectModel.get(selectIndex).value
            }
        }
    }

    function indexByValue(value) {
        for (var i = 0; i < effectModel.count; i++) {
            if (effectModel.get(i).value === value) {
                return i
            }
        }
        return 0
    }

    function update(osdType)
    {
        if (match(osdType)) {
            if (Panel.lastOsdType() === osdType) {
                Qt.callLater(function() {
                    control.selectIndex = (control.selectIndex + 1) % effectModel.count
                })
            }
            return true
        }
        return false
    }

    function match(osdType)
    {
        return osdType === "SwitchWM"
    }

    ListView {
        id: listview
        width: 410
        height: contentHeight
        anchors {
            margins: 10
            centerIn: parent
        }
        model: effectModel
        spacing: 10

        delegate: D.ItemDelegate {
            id: itemView

            padding: 0
            spacing: 0
            checkable: true
            width: listview.width

            required property int index
            required property string iconName
            required property string title
            required property string description
            property bool isCurrent: control.selectIndex === itemView.index

            property D.Palette backgroundColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.3)
                normalDark: Qt.rgba(0, 0, 0, 0.3)
            }
            property D.Palette checkedBackgroundColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.6)
                normalDark: Qt.rgba(0, 0, 0, 0.5)
            }

            contentItem: RowLayout {
                spacing: 0

                D.DciIcon {
                    sourceSize {
                        width: 32
                        height: 32
                    }
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.leftMargin: 18
                    name: itemView.iconName
                    theme: D.DTK.themeType
                    palette: D.DTK.makeIconPalette(control.palette)
                }

                ColumnLayout {
                    Layout.leftMargin: 18
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.topMargin: 18
                    Layout.bottomMargin: 18
                    spacing: 6
                    D.Label {
                        text: itemView.title
                        font {
                            family: D.DTK.fontManager.t5.family
                            pointSize: D.DTK.fontManager.t5.pointSize
                            bold: true
                        }
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                    }

                    D.Label {
                        text: itemView.description
                        font: D.DTK.fontManager.t6
                        palette.windowText: D.DTK.palette.windowText
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        Layout.maximumWidth: 298
                        wrapMode: Text.WordWrap
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                D.DciIcon {
                    sourceSize {
                        width: 16
                        height: 16
                    }
                    visible: itemView.isCurrent
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    Layout.rightMargin: 10
                    name: "item_checked"
                    theme: D.DTK.themeType
                    palette: D.DTK.makeIconPalette(control.palette)
                }
            }

            background: Item {
                Rectangle {
                    id: backgroundRect
                    anchors.fill: parent
                    radius: 6
                    color: itemView.isCurrent ? itemView.D.ColorSelector.checkedBackgroundColor
                                                                  : itemView.D.ColorSelector.backgroundColor
                }
                D.InsideBoxBorder {
                    property D.Palette insideBorderColor: D.Palette {
                        normal: Qt.rgba(1, 1, 1, 0.1)
                        normalDark: Qt.rgba(1, 1, 1, 0.05)
                    }
                    property D.Palette checkedInsideBorderColor: D.Palette {
                        normal: Qt.rgba(1, 1, 1, 0.15)
                        normalDark: Qt.rgba(1, 1, 1, 0.08)
                    }
                    radius: backgroundRect.radius
                    anchors.fill: parent
                    color: itemView.isCurrent ? D.ColorSelector.checkedInsideBorderColor
                                              : D.ColorSelector.insideBorderColor
                }
                D.OutsideBoxBorder {
                    property D.Palette outsideBorderColor: D.Palette {
                        normal: Qt.rgba(0, 0, 0, 0.05)
                        normalDark: Qt.rgba(0, 0, 0, 0.4)
                    }
                    property D.Palette checkedOutsideBorderColor: D.Palette {
                        normal: Qt.rgba(0, 0, 0, 0.1)
                        normalDark: Qt.rgba(0, 0, 0, 0.45)
                    }
                    radius: backgroundRect.radius
                    anchors.fill: parent
                    color: itemView.isCurrent ? D.ColorSelector.checkedOutsideBorderColor
                                              : D.ColorSelector.outsideBorderColor
                }
            }
        }
    }

    ListModel {
        id: effectModel
        ListElement {
            value: main.WindowEffectType.Normal
            iconName: "osd_screen_highperformance"
            title: qsTr("Optimal performance")
            description: qsTr("Disable all interface and window effects for efficient system performance")
        }
        ListElement {
            value: main.WindowEffectType.Better
            iconName: "osd_screen_balance"
            title: qsTr("Balance")
            description: qsTr("Limit some window effects for excellent visuals while maintaining smooth system performance")
        }
        ListElement {
            value: main.WindowEffectType.Best
            iconName: "osd_screen_bestvisual"
            title: qsTr("Best Visuals")
            description: qsTr("Enable all interface and window effects for the best visual experience")
        }
    }
}
