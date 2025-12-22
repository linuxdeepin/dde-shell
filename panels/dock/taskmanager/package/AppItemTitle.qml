// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.ds.dock.taskmanager 1.0
import org.deepin.dtk 1.0 as D

Item {
    id: root

    property bool active: titleLoader.active
    property string text: ""

    implicitWidth: titleLoader.width
    implicitHeight: titleLoader.height

    TextCalculator.text: root.text

    Loader {
        id: titleLoader
        active: root.enabled && root.TextCalculator.elidedText.length > 0
        sourceComponent: Text {
            id: titleText

            text: root.TextCalculator.elidedText
            
            color: D.DTK.themeType === D.ApplicationHelper.DarkType ? "#FFFFFF" : "#000000"            
            font: root.TextCalculator.calculator.font
            verticalAlignment: Text.AlignVCenter

            opacity: visible ? 1.0 : 0.0
            
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
    }
}
