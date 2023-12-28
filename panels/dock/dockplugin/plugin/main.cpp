// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockpluginmanagerintegration_p.h"

#include <QtWaylandClient/private/qwaylandshellintegrationplugin_p.h>

class DockPluginManagerIntegration : public QtWaylandClient::QWaylandShellIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandShellIntegrationFactoryInterface_iid FILE "dockplugin-shell.json")

public:
    DockPluginManagerIntegration()
    {}

    QtWaylandClient::QWaylandShellIntegration *create(const QString &key, const QStringList &paramList) override
    {
        Q_UNUSED(key);
        Q_UNUSED(paramList);
        return new dock::DockPluginManagerIntegration();
    }
};

#include "main.moc"
