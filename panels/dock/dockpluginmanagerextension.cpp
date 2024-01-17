// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockpluginmanagerextension_p.h"
#include "dockpluginmanagerintegration_p.h"

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandCompositor>

PluginSurface::PluginSurface(DockPluginManager* manager, const QString& pluginId, const QString& itemKey, DockPluginManager::SurfaceType surfaceType, QWaylandSurface *surface, const QWaylandResource &resource)
    : m_manager(manager)
    , m_pluginId(pluginId)
    , m_itemKey(itemKey)
    , m_surfaceType(surfaceType)
    , m_surface(surface)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();

    connect(m_surface, &QWaylandSurface::surfaceDestroyed, this, [this, manager](){
        Q_EMIT manager->pluginSurfaceDestroyed(this);
    });
}

QWaylandQuickShellIntegration* PluginSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new DockPluginManagerIntegration(item);
}

QWaylandSurface* PluginSurface::surface() const
{
    return m_surface;
}

QString PluginSurface::pluginId() const
{
    return m_pluginId;
}

QString PluginSurface::itemKey() const
{
    return m_itemKey;
}

QString PluginSurface::contextMenu() const
{
    return m_contextMenu;
}

int32_t PluginSurface::pluginFlags() const
{
    return m_flags;
}

DockPluginManager::SurfaceType PluginSurface::surfaceType() const
{
    return m_surfaceType;
}

void PluginSurface::click(const QString &menuId, uint32_t checked)
{
    send_handle_click(menuId, checked);
}

void PluginSurface::dock_plugin_surface_request_set_applet_visible(Resource *resource, const QString &itemKey, uint32_t visible)
{

}

void PluginSurface::dock_plugin_surface_create_context_menu(Resource *resource, const QString &contextMenu)
{
    if (contextMenu != m_contextMenu) {
        m_contextMenu = contextMenu;   
    }
}

void PluginSurface::dock_plugin_surface_create_dcc_icon(Resource *resource, const QString &dccIcon)
{
    if (dccIcon != m_dccIcon) {
        m_dccIcon = dccIcon;
    }
}

void PluginSurface::dock_plugin_surface_plugin_flags(Resource *resource, int32_t flags)
{
    if (flags != m_flags) {
        m_flags = flags;
    }
}


DockPluginManager::DockPluginManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
    , m_dockDisplayMode(0)
    , m_dockPosition(0)
{
}

void DockPluginManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    init(compositor->display(), 1);
}

uint32_t DockPluginManager::dockPosition() const
{
    return m_dockPosition;
}

void DockPluginManager::setDockPosition(uint32_t position)
{
    if (m_dockPosition == position)
        return;
    m_dockPosition = position;

    foreach (QWaylandSurface* surface, m_pluginSurfaces) {
        Resource *target = resourceMap().value(surface->waylandClient());
        if (target) {
            send_position_changed(target->handle, m_dockPosition);
        }
    }
}

uint32_t DockPluginManager::dockDisplayMode() const
{
    return  m_dockDisplayMode;
}

void DockPluginManager::setDockDisplayMode(uint32_t displayMode)
{
    if (m_dockDisplayMode == displayMode)
        return;
    m_dockDisplayMode = displayMode;

    foreach (QWaylandSurface* surface, m_pluginSurfaces) {
        Resource *target = resourceMap().value(surface->waylandClient());
        if (target) {
            send_display_mode_changed(target->handle, m_dockDisplayMode);
        }
    }
}

uint32_t DockPluginManager::dockColorTheme() const
{
    return m_dockColorTheme;
}

void DockPluginManager::setDockColorTheme(uint32_t type)
{
    if (type == m_dockColorTheme)
        return;
    m_dockColorTheme = type;

    foreach (QWaylandSurface* surface, m_pluginSurfaces) {
        Resource *target = resourceMap().value(surface->waylandClient());
        if (target) {
            send_color_theme_changed(target->handle, m_dockColorTheme);
        }
    }
}

void DockPluginManager::dock_plugin_manager_v1_create_plugin_surface(Resource *resource, const QString &pluginId, const QString &itemKey, uint32_t surfaceType, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    m_pluginSurfaces << qwaylandSurface;

    connect(qwaylandSurface, &QWaylandSurface::surfaceDestroyed, this, [this, qwaylandSurface](){
        m_pluginSurfaces.removeAll(qwaylandSurface);
    });

    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::dock_plugin_surface_interface,
                                                           wl_resource_get_version(resource->handle), id));

    QMetaObject::invokeMethod(this, [this, resource](){
        send_position_changed(resource->handle, m_dockPosition);
        send_color_theme_changed(resource->handle, m_dockColorTheme);
        send_display_mode_changed(resource->handle, m_dockDisplayMode);
    }, Qt::QueuedConnection);

    auto plugin = new PluginSurface(this, pluginId, itemKey, static_cast<SurfaceType>(surfaceType), qwaylandSurface, shellSurfaceResource);
    Q_EMIT pluginSurfaceCreated(plugin);
}
