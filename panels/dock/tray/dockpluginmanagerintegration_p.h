// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtWaylandCompositor/private/qwaylandquickshellsurfaceitem_p.h>

class PluginSurface;

class DockPluginManagerIntegration : public QWaylandQuickShellIntegration
{
    Q_OBJECT
public:
    DockPluginManagerIntegration(QWaylandQuickShellSurfaceItem *item);
    ~DockPluginManagerIntegration() override;

private Q_SLOTS:
    void handleDockPluginSurfaceDestroyed();

private:
    QWaylandQuickShellSurfaceItem *m_item = nullptr;
    PluginSurface *m_pluginSurface = nullptr;
};
