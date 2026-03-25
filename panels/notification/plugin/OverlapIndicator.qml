// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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
    property bool clipItems: false

    implicitHeight: layout.height
    implicitWidth: 360
    property Component background: NotifyItemBackground {
                        radius: parent ? parent.radius : root.radius
                        opacity: 0
                    }

    ColumnLayout {
        id: layout
        spacing: -1
        Repeater {
            model: root.count
            delegate: Item {
                id: item
                readonly property int realIndex: revert ? count - index - 1 : index
                Layout.preferredHeight: root.radius
                Layout.preferredWidth: root.width - (realIndex) * radius * 2
                Layout.alignment: Qt.AlignHCenter
                z: -realIndex
                clip: root.clipItems

                Loader {
                    id: contentLoader
                    width: parent.width
                    height: root.radius * 2
                    property int overlapHeight: root.overlapHeight
                    property int radius: root.radius
                    property alias realIndex: item.realIndex
                    property bool revert: root.revert
                    anchors {
                        top: revert ? parent.top : undefined
                        bottom: revert ? undefined : parent.bottom
                    }
                    sourceComponent: root.background

                    onLoaded: function() {
                        contentLoader.item.realIndex = Qt.binding(function() { return item.realIndex; })
                        if (root.enableAnimation) {
                            fadeInAnimation.start()
                        } else {
                            contentLoader.item.opacity = 1
                        }
                    }

                    SequentialAnimation {
                        id: fadeInAnimation

                        PauseAnimation {
                            duration: realIndex * 100
                        }

                        ParallelAnimation {
                            NumberAnimation {
                                target: contentLoader.item
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
