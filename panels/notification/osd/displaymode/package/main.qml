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
                Applet.next()
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
                normal: Qt.rgba(1, 1, 1, 0.4)
                normalDark: Qt.rgba(0, 0, 0, 0.4)
            }
            property D.Palette checkedBackgroundColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.6)
                normalDark: Qt.rgba(0, 0, 0, 0.6)
            }
            property D.Palette dropShadowColor: D.Palette {
                normal: Qt.rgba(0, 0, 0, 0.1)
                normalDark: Qt.rgba(1, 1, 1, 0.1)
            }
            property D.Palette innerShadowColor:  D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.2)
                normalDark: Qt.rgba(0, 0, 0, 0.2)
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
                    color: Applet.currentPlanItem && Applet.currentPlanItem.key === model.key ? D.DTK.platformTheme.activeColor : palette.windowText
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
                D.BoxShadow {
                    anchors.fill: parent
                    shadowOffsetX: 0
                    shadowOffsetY: 1
                    shadowColor: itemView.D.ColorSelector.dropShadowColor
                    shadowBlur: 1
                    cornerRadius: backgroundRect.radius
                    spread: 0
                    hollow: true
                }
                D.BoxInsetShadow {
                    anchors.fill: parent
                    shadowOffsetX: 0
                    shadowOffsetY: 1
                    shadowBlur: 1
                    spread: 0
                    cornerRadius: backgroundRect.radius
                    shadowColor: itemView.D.ColorSelector.innerShadowColor
                }
            }
        }
    }
}
