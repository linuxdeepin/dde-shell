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
    implicitWidth: view.width + view.anchors.leftMargin + view.anchors.rightMargin
    implicitHeight: view.height + view.anchors.topMargin + view.anchors.bottomMargin

    readonly property bool singleView: false

    function update(osdType)
    {
        if (match(osdType)) {
            Applet.sync()
            if (Panel.lastOsdType() === osdType) {
                Applet.next()
            }
            return true
        }
        return false
    }
    function match(osdType)
    {
        return osdType === "SwitchLayout"
    }

    ListView {
        id: view
        anchors {
            margins: 10
            centerIn: parent
        }
        height: contentHeight
        model: Applet.layouts
        spacing: 6
        function maxItemWidth()
        {
            var maxWidth = 120
            for (var i = 0; i < view.count; ++i) {
                var item = view.itemAtIndex(i)
                if (item && item.implicitWidth > maxWidth)
                    maxWidth = item.implicitWidth
            }
            return maxWidth
        }
        implicitWidth: maxItemWidth()

        delegate: D.ItemDelegate {
            id: itemView
            width: view.width
            height: 48
            palette.windowText: undefined
            spacing: 0
            padding: 0

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
                normalDark: Qt.rgba(0, 0, 0, 0.7)
            }
            property D.Palette innerShadowColor: D.Palette {
                normal: Qt.rgba(1, 1, 1, 0.2)
                normalDark: Qt.rgba(1, 1, 1, 0.03)
            }

            property bool isCurrent: Applet.currentLayout === model.key

            contentItem: RowLayout {
                spacing: 0

                Text {
                    font: D.DTK.fontManager.t4
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    Layout.leftMargin: 15
                    horizontalAlignment: Text.AlignVCenter
                    text: model.text
                    color: palette.windowText
                }

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    Layout.rightMargin: 10
                    D.DciIcon {
                        sourceSize {
                            width: 16
                            height: 16
                        }
                        visible: itemView.isCurrent
                        name: "item_checked"
                        theme: D.DTK.themeType
                        palette: D.DTK.makeIconPalette(control.palette)
                    }
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
            Component.onCompleted: {
                Qt.callLater(function () {
                    view.implicitWidth = view.maxItemWidth()
                })
            }
        }
    }
}
