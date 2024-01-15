// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockplugin.h"
#include "dockpluginsurface_p.h"
#include "dockpluginmanager_p.h"

#include "qwayland-dock-plugin-manager-v1.h"

namespace dock {
DockPluginSurface::DockPluginSurface(DockPluginManager *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::dock_plugin_surface()
    , m_plugin(DockPlugin::get(window->window()))
{
    init(manager->create_plugin_surface(m_plugin->pluginId(), m_plugin->itemKey(), m_plugin->pluginType(), window->wlSurface()));
    connect(manager, &DockPluginManager::dockPositionChnaged, m_plugin, &DockPlugin::dockPositionChanged);
    connect(manager, &DockPluginManager::dockColorThemeChanged, m_plugin, &DockPlugin::dockColorThemeChanged);
    connect(manager, &DockPluginManager::dockDisplayModeChanged, m_plugin, &DockPlugin::dockDisplayModeChanged);
}

DockPluginSurface::~DockPluginSurface()
{}

void DockPluginSurface::onRequestSetAppletVisible(const QString &itemKey, uint32_t visible)
{
    request_set_applet_visible(itemKey, visible);
}

void DockPluginSurface::onContextMenuCreated(const QString &contextMenu)
{
    create_context_menu(contextMenu);
}

void DockPluginSurface::onDCCIconChanged(const QString &dccIcon)
{
    create_dcc_icon(dccIcon);
}

void DockPluginSurface::onPluginFlags(int32_t flags)
{
    plugin_flags(flags);
}

void DockPluginSurface::dock_plugin_surface_configure(int32_t width, int32_t height)
{

}

void DockPluginSurface::dock_plugin_surface_handle_click(const QString &menuId, uint32_t checked)
{
    Q_EMIT m_plugin->clicked(menuId, checked);
}
}
