// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.dtk.style 1.0 as DStyle
import org.deepin.ds.notification

NotifyItem {
    id: root
    implicitWidth: impl.implicitWidth
    implicitHeight: impl.implicitHeight
    property bool closeVisible: activeFocus || impl.hovered
    property int miniContentHeight: NotifyStyle.contentItem.miniHeight
    property bool enableDismissed: true
    property alias clearButton: clearLoader.sourceComponent

    Control {
        id: impl
        anchors.fill: parent

        Item {
            anchors.fill: parent
            z: -1
            enabled: root.enableDismissed
            TapHandler {
                property bool isLongPressed
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onCanceled: function () {
                    if (isLongPressed) {
                        console.log("Dissmiss notify", root.appName, isLongPressed)
                        root.dismiss()
                    }
                    isLongPressed = false
                }
                onLongPressed: isLongPressed = true
                onTapped: function () {
                    if (!root.defaultAction)
                        return

                    console.log("Click default action notify", root.appName, root.defaultAction)
                    root.actionInvoked(root.defaultAction)
                }
            }
        }

        // placeHolder to receive MouseEvent
        Control {
            id: closePlaceHolder
            anchors {
                top: parent.top
                topMargin: -height / 2
                right: parent.right
                rightMargin: -width / 2
            }
            width: 20
            height: 20

            Loader {
                id: clearLoader
                focus: true
                anchors.right: parent.right
                active: !(root.strongInteractive && root.actions.length > 0) && (root.closeVisible || closePlaceHolder.hovered || activeFocus)
                sourceComponent: SettingActionButton {
                    id: closeBtn
                    objectName: "closeNotify-" + root.appName
                    icon.name: "clean-alone"
                    padding: 2
                    forcusBorderVisible: visualFocus
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
                                // TODO crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.5)
                                crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 1.0)
                            }
                            normalDark {
                                // TODO crystal: Qt.rgba(240 / 255.0, 240 / 255.0, 240 / 255.0, 0.5)
                                crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 1.0)
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

        contentItem: RowLayout {
            spacing: 0
            Binding {
                target: root
                property: "miniContentHeight"
                value: 26 + NotifyStyle.contentItem.topMargin + NotifyStyle.contentItem.bottomMargin
            }

            DciIcon {
                id: appIcon
                name: root.iconName !== "" ? root.iconName : "application-x-desktop"
                sourceSize: Qt.size(24, 24)
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.topMargin: 8
                Layout.leftMargin: 10
                palette: DTK.makeIconPalette(impl.palette)
                theme: impl.ColorSelector.controlTheme
            }

            ColumnLayout {
                id: contentLayout
                spacing: 0
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.rightMargin: 10
                Layout.leftMargin: 10
                Layout.topMargin: NotifyStyle.contentItem.topMargin
                Layout.bottomMargin: NotifyStyle.contentItem.bottomMargin
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: NotifyStyle.contentItem.miniHeight
                Layout.maximumHeight: 240
                RowLayout {
                    id: firstLine
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
                        active: !root.closeVisible && !closePlaceHolder.hovered
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
                    id: bodyRow
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Text {
                        id: bodyText
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        // text 宽度若让Layout通过implicitWidth计算会导致ListView的add动画出现位置错误，故这里手动计算Text的宽度
                        Layout.preferredWidth: NotifyStyle.contentItem.width - appIcon.width
                            - appIcon.Layout.leftMargin - appIcon.Layout.rightMargin
                            - contentLayout.Layout.rightMargin - contentLayout.Layout.leftMargin 
                            - (contentIconLoader.active ? (contentIconLoader.width + 1) : 0)
                            - bodyRow.spacing * bodyRow.children.length - 1
                        visible: text !== ""
                        text: root.content
                        maximumLineCount: root.contentRowCount
                        font: DTK.fontManager.t8
                        color: palette.windowText
                        wrapMode: Text.Wrap
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
                        id: contentIconLoader
                        Layout.maximumWidth: 106
                        Layout.maximumHeight: 106
                        Layout.minimumWidth: 16
                        Layout.minimumHeight: 16
                        Layout.alignment: Qt.AlignRight
                        active: root.contentIcon !== ""
                        visible: active
                        // TODO DciIcon's bounding can't be limit by maximumWidth.
                        sourceComponent: Image {
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: root.contentIcon
                        }
                    }
                }

                Loader {
                    active: root.actions.length > 0
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

        background: NotifyItemBackground { }
    }
}
