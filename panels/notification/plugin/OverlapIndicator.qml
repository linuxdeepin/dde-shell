// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification

Item {
    id: root
    required property var count
    property int radius: 12
    property int overlapHeight: 8
    property bool revert: false
    property bool enableAnimation: false

    implicitHeight: layout.height
    implicitWidth: 360
    ColumnLayout {
        id: layout
        spacing: 0
        Repeater {
            model: root.count
            delegate: Item {
                id: item
                readonly property int realIndex: revert ? count - index - 1 : index
                Layout.preferredHeight: overlapHeight + 2
                Layout.preferredWidth: root.width - (realIndex) * radius *2
                Layout.alignment: Qt.AlignHCenter
                z: -realIndex

                NotifyItemBackground {
                    id: background
                    radius: root.radius
                    width: parent.width
                    height: radius * 2
                    anchors {
                        top: revert ? undefined : parent.top
                        topMargin: revert ? undefined : -(height - overlapHeight)
                        bottomMargin: revert ? -(height - overlapHeight) : undefined
                    }
                    opacity: 0

                    Component.onCompleted: {
                        if (root.enableAnimation) {
                            fadeInAnimation.start()
                        } else {
                            opacity = 1
                        }
                    }

                    SequentialAnimation {
                        id: fadeInAnimation

                        PauseAnimation {
                            duration: realIndex * 100
                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: background
                                property: "opacity"
                                from: 0
                                to: 1
                                duration: 300
                                easing.type: Easing.OutQuad
                            }
                        }
                    }
                }
            }
        }
    }
}
