// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.ds 1.0

ContainmentItem {
    id: root
    objectName: "containment item"
    RowLayout {
        Repeater {
            model: Applet.appletItems
            delegate: Control {
                contentItem: modelData
            }
        }
    }
}
