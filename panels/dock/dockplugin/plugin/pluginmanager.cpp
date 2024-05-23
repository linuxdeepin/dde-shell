// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plugin.h"
#include "pluginmanager_p.h"
#include "pluginsurface_p.h"

namespace Plugin {
PluginManager::PluginManager(::wl_registry *registry, uint32_t id, uint32_t version)
    : QtWayland::plugin_manager_v1(registry, id, version)
    , QObject()
{
    init(registry, id, version);
}

PluginManager::~PluginManager()
{
}

void PluginManager::plugin_manager_v1_event_message(const QString &msg)
{
    Q_UNUSED(msg);
}

QtWaylandClient::QWaylandShellSurface* PluginManager::createPluginSurface(QtWaylandClient::QWaylandWindow *window)
{
    if (EmbemdPlugin::contains(window->window())) {
        return new PluginSurface(this, window);
    }

    if (PluginPopup::contains(window->window())) {
        return new PluginPopupSurface(this, window);
    }

    Q_UNREACHABLE();
}
}
