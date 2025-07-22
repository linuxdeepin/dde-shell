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
    implicitWidth: childrenRect.width
    implicitHeight: 60

    readonly property bool singleView: true

    function update(osdType)
    {
        if (match(osdType)) {
            selectModleByType(osdType)
            return true
        }
        return false
    }
    function match(osdType)
    {
        for (var i = 0; i < osdTypeModel.count; i++) {
            var item = osdTypeModel.get(i)
            if (item.type === osdType) {
                return true
            }
        }
        return false
    }

    function selectModleByType(osdType)
    {
        for (var i = 0; i < osdTypeModel.count; i++) {
            var item = osdTypeModel.get(i)
            if (item.type === osdType) {
                iconName = item.iconName
                text = item.text
                break
            }
        }
    }

    property string iconName
    property string text
    ListModel {
        id: osdTypeModel

        ListElement { type: "WLANOn"; iconName: "osd_wifi_on"; text: qsTr("WLAN on")}
        ListElement { type: "WLANOff"; iconName: "osd_wifi_off"; text: qsTr("WLAN off")}
        ListElement { type: "CapsLockOn"; iconName: "osd_capslock_on"; text: qsTr("Caps Lock on")} // 大写
        ListElement { type: "CapsLockOff"; iconName: "osd_capslock_off"; text: qsTr("Caps Lock off")} // 小写
        ListElement { type: "NumLockOn"; iconName: "osd_numeric_keypad_on"; text: qsTr("Numeric keypad on")} // 数字键盘开启
        ListElement { type: "NumLockOff"; iconName: "osd_numeric_keypad_off"; text: qsTr("Numeric keypad off")} // 数字键盘关闭
        ListElement { type: "TouchpadOn"; iconName: "osd_touchpad_on"; text: qsTr("Touchpad on")} // 触摸板开启
        ListElement { type: "TouchpadOff"; iconName: "osd_touchpad_off"; text: qsTr("Touchpad off")} // 触摸板关闭
        ListElement { type: "TouchpadToggle"; iconName: "osd_touchpad_exchange"; text: qsTr("Touchpad toggle")} // 触控板切换
        ListElement { type: "FnToggle"; iconName: "osd_fn"; text: qsTr("Fn toggle")} // Fn切换
        ListElement { type: "AirplaneModeOn"; iconName: "osd_airplane_on"; text: qsTr("Airplane mode on")}
        ListElement { type: "AirplaneModeOff"; iconName: "osd_airplane_off"; text: qsTr("Airplane mode off")}
        ListElement { type: "AudioMicMuteOn"; iconName: "osd_mic_off"; text: qsTr("Microphone off")}
        ListElement { type: "AudioMicMuteOff"; iconName: "osd_mic_on"; text: qsTr("Microphone on")}
        ListElement { type: "balance"; iconName: "osd_power_balance"; text: qsTr("Balanced power")}
        ListElement { type: "powersave"; iconName: "osd_power_save"; text: qsTr("Power saver")}
        ListElement { type: "performance"; iconName: "osd_power_performance"; text: qsTr("High performance")}
        ListElement { type: "SwitchWM3D"; iconName: "osd_wm_3d"; text: qsTr("Window effect enabled")}
        ListElement { type: "SwitchWM2D"; iconName: "osd_wm_2d"; text: qsTr("Window effect disabled")}
        ListElement { type: "SwitchWMError"; iconName: "osd_wm_failed"; text: qsTr("Failed to enable window effects")}
    }

    RowLayout {
        spacing: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.topMargin: 2

        D.DciIcon {
            sourceSize {
                width: 32
                height: 32
            }
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.leftMargin: 20
            visible: control.iconName
            name: control.iconName
            theme: D.DTK.themeType
            palette: D.DTK.makeIconPalette(control.palette)
        }

        Text {
            Layout.leftMargin: 16
            font: D.DTK.fontManager.t4
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.rightMargin: 20
            visible: control.text
            color: palette.windowText
            text: control.text
        }
    }
}
