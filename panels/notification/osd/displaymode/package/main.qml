// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: control
    implicitWidth: 370
    implicitHeight: view.height + view.anchors.topMargin + view.anchors.bottomMargin

    readonly property bool singleView: false

    function update(osdType)
    {
        Applet.sync()
        if (match(osdType)) {

            if (osdType === "DirectSwitchLayout") {
                if (Applet.state !== 2) {
                    Applet.doAction()
                }
            } else if (osdType === "SwitchMonitors") {
                if (Panel.lastOsdType() === osdType) {
                    Applet.next()
                }
            }

            return true
        }
        return false
    }
    function match(osdType)
    {
        return osdType === "SwitchMonitors" || osdType === "DirectSwitchLayout"
    }

    ListView {
        id: view
        width: 350
        height: contentHeight
        anchors {
            margins: 10
            centerIn: parent
        }

        model: Applet.planItems
        spacing: 10

        delegate: D.ItemDelegate {
            id: itemView
            height: 60
            width: view.width
            palette.windowText: undefined

            property D.Palette backgroundColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.3)
                normalDark: Qt.rgba(0, 0, 0, 0.3)
            }
            property D.Palette checkedBackgroundColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.6)
                normalDark: Qt.rgba(0, 0, 0, 0.5)
            }

            property bool isCurrent: Applet.currentPlanItem && Applet.currentPlanItem.key === model.key

            contentItem: RowLayout {
                spacing: 0

                D.DciIcon {
                    sourceSize {
                        width: 32
                        height: 32
                    }
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: 14
                    name: model.iconName
                    theme: D.DTK.themeType
                    palette: D.DTK.makeIconPalette(control.palette)
                }

                Text {
                    Layout.leftMargin: 20
                    font: D.DTK.fontManager.t5
                    Layout.alignment: Qt.AlignVCenter
                    text: model.text
                    color: palette.windowText
                }

                Item {
                    Layout.fillWidth: true
                }

                D.DciIcon {
                    sourceSize {
                        width: 16
                        height: 16
                    }
                    visible: Applet.currentPlanItem && Applet.currentPlanItem.key === model.key
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    Layout.rightMargin: 14
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
}
