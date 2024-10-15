// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root

    property bool closeVisible: root.hovered || root.activeFocus
    // placeHolder to receive MouseEvent
    Control {
        id: closePlaceHolder
        focus: true
        anchors {
            top: parent.top
            topMargin: -height / 2
            right: parent.right
            rightMargin: -width / 2
        }
        width: 20
        height: 20
        contentItem: Loader {
            active: root.closeVisible || closePlaceHolder.hovered || closePlaceHolder.activeFocus || activeFocus
            sourceComponent: SettingActionButton {
                id: closeBtn
                objectName: "closeNotify-" + root.appName
                icon.name: "clean-alone"
                padding: 2
                forcusBorderVisible: visualFocus || closePlaceHolder.visualFocus
                onClicked: function () {
                    root.remove()
                }
                background: BoxPanel {
                    radius: closeBtn.radius
                    enableBoxShadow: true
                    boxShadowBlur: 4
                    boxShadowOffsetY: 1
                    color1: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.5)
                        }
                        normalDark {
                            crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 0.5)
                        }
                    }
                    color2: color1
                    insideBorderColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.2)
                        }
                        normalDark {
                            crystal: Qt.rgba(255 / 255.0, 255 / 255.0, 255 / 255.0, 0.1)
                        }
                    }
                    outsideBorderColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(0, 0, 0, 0.08)
                        }
                        normalDark {
                            crystal: Qt.rgba(0, 0, 0, 0.4)
                        }
                    }
                    dropShadowColor: Palette {
                        normal {
                            common: ("transparent")
                            crystal: Qt.rgba(0, 0, 0, 0.15)
                        }
                        normalDark {
                            crystal: Qt.rgba(0, 0, 0, 0.4)
                        }
                    }
                    innerShadowColor1: null
                    innerShadowColor2: innerShadowColor1
                }
            }
        }
    }
    Loader {
        id: actionPlaceHolder
        anchors {
            bottom: parent.bottom
            bottomMargin: 8
            right: parent.right
            rightMargin: 8
        }

        active: !root.strongInteractive && root.actions.length > 0
        visible: active
        sourceComponent: NotifyAction {
            actions: root.actions
            onActionInvoked: function (actionId) {
                root.actionInvoked(actionId)
            }
            background: NotifyItemBackground {
                radius: 6
                implicitHeight: 30
                implicitWidth: 50
                outsideBorderColor: null
                insideBorderColor: null
            }
        }
    }

    contentItem: RowLayout {
        spacing: 0
        DciIcon {
            name: root.iconName
            sourceSize: Qt.size(24, 24)
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.topMargin: 8
            Layout.leftMargin: 10
            palette: DTK.makeIconPalette(root.palette)
            mode: root.ColorSelector.controlState
            theme: root.ColorSelector.controlTheme
        }

        ColumnLayout {
            spacing: 0
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.rightMargin: 10
            Layout.leftMargin: 10
            Layout.topMargin: 4
            Layout.bottomMargin: 8
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 40
            Layout.maximumHeight: 240
            RowLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Text {
                    text: root.appName
                    font: DTK.fontManager.t10
                    color: palette.windowText
                }

                Item {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                }

                Loader {
                    id: time
                    active: !root.closeVisible
                    visible: active
                    Layout.alignment: Qt.AlignRight
                    sourceComponent: Text {
                        text: root.date
                        font: DTK.fontManager.t10
                        color: palette.windowText
                    }
                }
            }

            Text {
                text: root.title
                visible: text !== ""
                maximumLineCount: 1
                font {
                    pixelSize: DTK.fontManager.t8.pixelSize
                    family: DTK.fontManager.t8.family
                    bold: true
                }
                color: palette.windowText
                wrapMode: Text.NoWrap
                elide: Text.ElideMiddle
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Text {
                    id: bodyText
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: root.content
                    maximumLineCount: 6
                    font: DTK.fontManager.t8
                    color: palette.windowText
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    linkColor: palette.highlight
                    onLinkActivated: function (link) {
                        root.linkActivated(link)
                    }
                    HoverHandler {
                        enabled: bodyText.hoveredLink !== ""
                        cursorShape: Qt.PointingHandCursor
                    }
                }
                Item {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                }

                Loader {
                    Layout.maximumWidth: 106
                    Layout.maximumHeight: 106
                    Layout.minimumWidth: 16
                    Layout.minimumHeight: 16
                    Layout.alignment: Qt.AlignRight
                    active: root.contentIcon !== ""
                    // TODO DciIcon's bounding can't be limit by maximumWidth.
                    sourceComponent: Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        source: root.contentIcon
                    }
                }
            }

            Loader {
                active: root.strongInteractive && root.actions.length > 0
                visible: active
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                sourceComponent: NotifyAction {
                    actions: root.actions
                    onActionInvoked: function (actionId) {
                        root.actionInvoked(actionId)
                    }
                }
            }
        }
    }

    background: NotifyItemBackground {
    }
}
