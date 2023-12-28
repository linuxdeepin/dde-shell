// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockpluginmanagerextension_p.h"
#include "dockpluginmanagerintegration_p.h"

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>

DockPluginManagerIntegration::DockPluginManagerIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
    , m_pluginSurface(qobject_cast<PluginSurface *>(item->shellSurface()))
{
    m_item->setSurface(m_pluginSurface->surface());
    connect(m_pluginSurface, &QWaylandShellSurface::destroyed,
            this, &DockPluginManagerIntegration::handleDockPluginSurfaceDestroyed);
}

DockPluginManagerIntegration::~DockPluginManagerIntegration()
{
    m_item->setSurface(nullptr);
}

void DockPluginManagerIntegration::handleDockPluginSurfaceDestroyed()
{
    m_pluginSurface = nullptr;
}
