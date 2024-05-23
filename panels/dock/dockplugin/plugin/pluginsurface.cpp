// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plugin.h"
#include "pluginsurface_p.h"
#include "pluginmanager_p.h"

#include "qwayland-plugin-manager-v1.h"

namespace Plugin {
PluginSurface::PluginSurface(PluginManager *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::plugin()
    , m_plugin(EmbemdPlugin::get(window->window()))
    , m_window(window->window())
{
    init(manager->create_plugin(m_plugin->pluginId(), m_plugin->itemKey(),m_plugin->pluginFlags(), m_plugin->pluginType(), window->wlSurface()));
}

PluginSurface::~PluginSurface()
{
}

void PluginSurface::plugin_close()
{
    m_window->hide();
}

void PluginSurface::plugin_pos(int32_t x, int32_t y)
{
    m_window->setX(x), m_window->setY(y);
}

PluginPopupSurface::PluginPopupSurface(PluginManager *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::plugin_popup()
    , m_popup(PluginPopup::get(window->window()))
    , m_window(window->window())
{
    init(manager->create_popup_at(m_popup->x(), m_popup->y(), m_popup->popupType(), window->wlSurface()));
}

PluginPopupSurface::~PluginPopupSurface()
{
}

void PluginPopupSurface::plugin_popup_close()
{
    m_window->hide();
}

}
