// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.ds.dock 1.0
import org.deepin.dtk

AppletItemBackground {
    id: control
    property bool splitBackgroundVisible: false
    property int windowCount: 0
    property int displayMode: Dock.Efficient
    property Palette minDockEffectActive: Palette {
        normal {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.8)
        }
        normalDark {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.6)
        }
        hovered {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.9)
        }
        hoveredDark {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.7)
        }
    }
    property Palette normalDockEffectActive: Palette {
        normal {
            crystal: ("transparent")
        }
        normalDark: normal
        hovered {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.15)
        }
        hoveredDark: hovered
    }
    backgroundColor: Palette {
        normal {
            crystal: if (displayMode === Dock.Efficient && control.windowCount > 0) {
                    return Qt.rgba(1.0, 1.0, 1.0, 0.35)
            } else {
                    return ("transparent")
            }
        }
        normalDark: {
            crystal: if (displayMode === Dock.Efficient && control.windowCount > 0) {
                    return Qt.rgba(1.0, 1.0, 1.0, 0.1)
            } else {
                    return ("transparent")
            }
        }
        hovered {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.15)
        }
        hoveredDark {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.15)
        }
        pressed {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.25)
        }
        pressedDark {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.25)
        }
    }
    activeBackgroundColor: displayMode === Dock.Efficient ? minDockEffectActive : normalDockEffectActive
    insideBorderColor: Palette {
        normal {
            crystal: if (displayMode === Dock.Efficient && control.windowCount > 0) {
                    return Qt.rgba(1.0, 1.0, 1.0, 0.05)
            } else {
                    return ("transparent")
            }
        }
        normalDark: normal
        hovered {
            crystal: Qt.rgba(0, 0, 0, 0.05)
        }
        hoveredDark: hovered
        pressed: hovered
        pressedDark: pressed
    }

    activeInsideBorderColor: Palette {
        normal {
            crystal: if (displayMode === Dock.Efficient) {
                    return Qt.rgba(0, 0, 0, 0.1)
            } else {
                    return ("transparent")
            }
        }
        normalDark: {
            crystal: if (displayMode === Dock.Efficient) {
                    return Qt.rgba(1.0, 1.0, 1.0, 0.1)
            } else {
                    return ("transparent")
            }
        }
        hovered {
            crystal: Qt.rgba(1.0, 1.0, 1.0, 0.1)
        }
        hoveredDark: hovered
        pressed: hovered
        pressedDark: pressed
    }
    outsideBorderColor: Palette {
        normal: {
            crystal: ("transparent")
        }
        normalDark: normal
        hovered : normal
        hoveredDark: hovered
        pressed: hovered
        pressedDark: pressed
    }
    activeOutsideBorderColor: Palette {
        normal {
            crystal: if (displayMode === Dock.Efficient) {
                    return Qt.rgba(0, 0, 0, 0.1)
            } else {
                    return ("transparent")
            }
        }
        normalDark {
            crystal: if (displayMode === Dock.Efficient) {
                    return Qt.rgba(0, 0, 0, 0.05)
            } else {
                    return ("transparent")
            }
        }
        hovered {
            crystal: Qt.rgba(0.0, 0.0, 0.0, 0.10)
        }
        hoveredDark: hovered
        pressed {
            crystal: Qt.rgba(0.0, 0.0, 0.0, 0.10)
        }
        pressedDark {
            crystal: Qt.rgba(0.0, 0.0, 0.0, 0.05)
        }
    }
}
