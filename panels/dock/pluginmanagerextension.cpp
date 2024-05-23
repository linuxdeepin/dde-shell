// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmanagerextension_p.h"
#include "pluginmanagerintegration_p.h"

#include <cstdint>

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandCompositor>

PluginSurface::PluginSurface(PluginManager* manager, const QString& pluginId, const QString& itemKey, int pluginType, QWaylandSurface *surface, const QWaylandResource &resource)
    : m_manager(manager)
    , m_pluginType(pluginType)
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
    return new PluginManagerIntegration(item);
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

uint32_t PluginSurface::pluginType() const
{
    return m_pluginType;
}

uint32_t PluginSurface::pluginFlags() const
{
    return m_flags;
}

void PluginSurface::updatePluginGeometry(const QRect &geometry)
{
    send_pos(geometry.x(), geometry.y());
}

PluginPopup::PluginPopup(PluginManager* manager, const int &x, const int &y, QWaylandSurface *surface, const QWaylandResource &resource)
    : m_manager(manager)
    , m_surface(surface)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();
}

QWaylandSurface* PluginPopup::surface() const
{
    return m_surface;
}

QWaylandQuickShellIntegration* PluginPopup::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new PluginPopupIntegration(item);
}

int32_t PluginPopup::x() const
{
    return m_x;
}

int32_t PluginPopup::y() const
{
    return m_y;
}

void PluginPopup::setX(int32_t x)
{
    m_x = x;
    Q_EMIT xChanged();
}

void PluginPopup::setY(int32_t y)
{
    m_y = y;
    Q_EMIT yChanged();
}

PluginManager::PluginManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
{
}

void PluginManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    init(compositor->display(), 1);
}

void PluginManager::plugin_manager_v1_request_message(Resource *resource, const QString &msg)
{
    qDebug() << msg;
}

void PluginManager::plugin_manager_v1_create_plugin(Resource *resource, const QString &pluginId, const QString &itemKey, int32_t plugin_flags, int32_t type, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    m_pluginSurfaces << qwaylandSurface;

    connect(qwaylandSurface, &QWaylandSurface::surfaceDestroyed, this, [this, qwaylandSurface](){
        m_pluginSurfaces.removeAll(qwaylandSurface);
    });

    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_interface,
                                                           wl_resource_get_version(resource->handle), id));

    auto plugin = new PluginSurface(this, pluginId, itemKey, type, qwaylandSurface, shellSurfaceResource);
    Q_EMIT pluginSurfaceCreated(plugin);
}

void PluginManager::plugin_manager_v1_create_popup_at(Resource *resource, int32_t x, int32_t y, int32_t type, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_popup_interface,
                                                           wl_resource_get_version(resource->handle), id));

    auto plugin = new PluginPopup(this, x, y, qwaylandSurface, shellSurfaceResource);
    plugin->setX(x), plugin->setY(y);
    Q_EMIT pluginPopupCreated(plugin);
}

