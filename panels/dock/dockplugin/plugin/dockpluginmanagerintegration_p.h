// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "dockpluginmanager_p.h"

#include <QtWaylandClient/private/qwaylandshellintegration_p.h>

namespace dock {
class DockPluginManagerIntegration : public QtWaylandClient::QWaylandShellIntegration
{
public:
    DockPluginManagerIntegration();
    ~DockPluginManagerIntegration() override;

    bool initialize(QtWaylandClient::QWaylandDisplay *display) override;
    QtWaylandClient::QWaylandShellSurface *createShellSurface(QtWaylandClient::QWaylandWindow *window) override;

private:
    static void registryPluginManager(void *data, struct wl_registry *registry, uint32_t id, const QString &interface, uint32_t version);
    QScopedPointer<DockPluginManager> m_pluginManager;
};
}
