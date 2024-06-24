import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

Item {
    id: pluginItem
    property var plugin: DockCompositor.findSurface(modelData.surfaceId)
    implicitHeight: surfaceItem.height
    implicitWidth: surfaceItem.width

    property var itemGlobalPoint: {
        var a = pluginItem
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x, y)
    }

    Rectangle {
        color: "transparent"
        anchors.fill: parent
        border.color: "red"
    }

    ShellSurfaceItem {
        id: surfaceItem
        anchors.centerIn: parent
        shellSurface: plugin
    }

    Component.onCompleted: {
        plugin.updatePluginGeometry(Qt.rect(itemGlobalPoint.x, itemGlobalPoint.y, surfaceItem.width, surfaceItem.height))
    }

    Timer {
        id: updatePluginItemGeometryTimer
        interval: 500
        running: false
        repeat: false
        onTriggered: {
            if (itemGlobalPoint.x > 0 && itemGlobalPoint.y > 0) {
                plugin.updatePluginGeometry(Qt.rect(itemGlobalPoint.x, itemGlobalPoint.y, surfaceItem.width, surfaceItem.height))
            }
        }
    }

    onItemGlobalPointChanged: {
        updatePluginItemGeometryTimer.start()
    }
}