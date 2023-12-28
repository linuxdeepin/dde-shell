// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockpluginmanagerintegration_p.h"
#include <qwayland-dock-plugin-manager-v1.h>

namespace dock {
DockPluginManagerIntegration::DockPluginManagerIntegration()
    : QWaylandShellIntegration()
{
}

DockPluginManagerIntegration::~DockPluginManagerIntegration()
{
}

bool DockPluginManagerIntegration::initialize(QtWaylandClient::QWaylandDisplay *display)
{
    QWaylandShellIntegration::initialize(display);
    display->addRegistryListener(registryPluginManager, this);
    return m_pluginManager != nullptr;
}

QtWaylandClient::QWaylandShellSurface *DockPluginManagerIntegration::createShellSurface(QtWaylandClient::QWaylandWindow *window)
{
    return m_pluginManager->createPluginSurface(window);
}

void DockPluginManagerIntegration::registryPluginManager(void *data, struct wl_registry *registry, uint32_t id, const QString &interface, uint32_t version)
{
    DockPluginManagerIntegration *shell = static_cast<DockPluginManagerIntegration *>(data);
    if (interface == dock_plugin_manager_v1_interface.name) {
        shell->m_pluginManager.reset(new DockPluginManager(registry, id, std::min(version, 1u)));   
    }
}
}
