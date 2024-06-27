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
    , m_plugin(EmbedPlugin::get(window->window()))
    , m_window(window->window())
{
    init(manager->create_plugin(m_plugin->pluginId(), m_plugin->itemKey(), m_plugin->displayName(), m_plugin->pluginFlags(), m_plugin->pluginType(), m_plugin->pluginSizePolicy(), window->wlSurface()));
    connect(manager, &PluginManager::dockPositionChanged, m_plugin, &EmbedPlugin::dockPositionChanged);
    connect(manager, &PluginManager::dockColorThemeChanged, m_plugin, &EmbedPlugin::dockColorThemeChanged);
    connect(manager, &PluginManager::eventMessage, m_plugin, &EmbedPlugin::eventMessage);

    connect(m_plugin, &EmbedPlugin::requestMessage, manager, [manager, this](const QString &msg) {
        manager->requestMessage(m_plugin->pluginId(), m_plugin->itemKey(), msg);
    });

    connect(m_plugin, &EmbedPlugin::pluginRecvMouseEvent, this, [this] (int type){
        mouse_event(type);
    });
}

PluginSurface::~PluginSurface()
{
}

void PluginSurface::plugin_close()
{
    m_window->hide();
}

void PluginSurface::plugin_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width > 0 && height > 0) {
        m_window->setGeometry(x, y, width, height);
    }

    Q_EMIT m_plugin->eventGeometry(QRect(x, y, width, height));
}

PluginPopupSurface::PluginPopupSurface(PluginManager *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::plugin_popup()
    , m_popup(PluginPopup::get(window->window()))
    , m_window(window->window())
{
    init(manager->create_popup_at(m_popup->pluginId(), m_popup->itemKey(), m_popup->popupType(), m_popup->x(), m_popup->y(), window->wlSurface()));
}

PluginPopupSurface::~PluginPopupSurface()
{
}

void PluginPopupSurface::plugin_popup_close()
{
    m_window->hide();
}

void PluginPopupSurface::plugin_popup_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto plugin = PluginPopup::get(m_window);
    if (plugin) {
        Q_EMIT plugin->eventGeometry(QRect(x, y, width, height));
    }
}

}
