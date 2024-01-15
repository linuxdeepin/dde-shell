// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <qwayland-dock-plugin-manager-v1.h>

#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace dock {
class PluginSurface;

class DockPluginManager : public QObject, public QtWayland::dock_plugin_manager_v1
{
    Q_OBJECT

public:
    DockPluginManager(::wl_registry *registry, uint32_t id, uint32_t version);
    ~DockPluginManager();

    QtWaylandClient::QWaylandShellSurface* createPluginSurface(QtWaylandClient::QWaylandWindow *window);

Q_SIGNALS:
    void dockPositionChnaged(uint32_t position);
    void dockDisplayModeChanged(uint32_t displayMode);
    void dockColorThemeChanged(uint32_t colorType);

protected:
    void dock_plugin_manager_v1_position_changed(uint32_t dockPosition) override;
    void dock_plugin_manager_v1_display_mode_changed(uint32_t dockDisplayMode) override;
    void dock_plugin_manager_v1_color_theme_changed(uint32_t dockColorType) override;

private:
    uint32_t m_dockPosition;
    uint32_t m_dockDisplayMode;
    uint32_t m_dockColorType;
};
}
