// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <qwayland-plugin-manager-v1.h>

#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace Plugin {

class PluginSurface;
class PluginManager : public QObject, public QtWayland::plugin_manager_v1
{
    Q_OBJECT

public:
    PluginManager(::wl_registry *registry, uint32_t id, uint32_t version);
    ~PluginManager();

    QtWaylandClient::QWaylandShellSurface* createPluginSurface(QtWaylandClient::QWaylandWindow *window);

protected:
    virtual void plugin_manager_v1_event_message(const QString &msg) override;

};
}
