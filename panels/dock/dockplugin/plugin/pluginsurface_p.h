// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "plugin.h"
#include "qwayland-plugin-manager-v1.h"

#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

namespace Plugin {

class PluginManager;
class PluginSurface : public QtWaylandClient::QWaylandShellSurface, public QtWayland::plugin
{
    Q_OBJECT

public:
    PluginSurface(PluginManager *manager, QtWaylandClient::QWaylandWindow *window);
    ~PluginSurface() override;

protected:
    virtual void plugin_close() override;
    virtual void plugin_pos(int32_t x, int32_t y) override;

private:
    EmbemdPlugin* m_plugin;
    QWindow* m_window;
};

class PluginPopupSurface : public QtWaylandClient::QWaylandShellSurface, public QtWayland::plugin_popup
{
    Q_OBJECT

public:
    PluginPopupSurface(PluginManager *manager, QtWaylandClient::QWaylandWindow *window);
    ~PluginPopupSurface() override;

protected:
    virtual void plugin_popup_close() override;

private:
    PluginPopup* m_popup;
    QWindow* m_window;
};

}
