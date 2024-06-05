// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmanagerextension_p.h"
#include "pluginmanagerintegration_p.h"
#include "constants.h"

#include <cstdint>

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandCompositor>

#include <QJsonObject>
#include <QJsonParseError>

PluginSurface::PluginSurface(PluginManager* manager, const QString& pluginId, const QString& itemKey, int pluginFlags, int pluginType, QWaylandSurface *surface, const QWaylandResource &resource)
    : m_manager(manager)
    , m_surface(surface)
    , m_itemKey(itemKey)
    , m_pluginId(pluginId)
    , m_flags(pluginFlags)
    , m_pluginType(pluginType)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();
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

void PluginSurface::setItemActive(bool isActive)
{
    if (m_isItemActive == isActive) {
        return;
    }

    m_isItemActive = isActive;
    Q_EMIT itemActiveChanged();
}

bool PluginSurface::isItemActive() const
{
    return m_isItemActive;
}

void PluginSurface::updatePluginGeometry(const QRect &geometry)
{
    send_geometry(geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

PluginPopup::PluginPopup(PluginManager* manager, int x, int y, int popupType, QWaylandSurface *surface, const QWaylandResource &resource)
    : m_manager(manager)
    , m_surface(surface)
    , m_popupType(popupType)
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

int32_t PluginPopup::popupType() const
{
    return m_popupType;
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

void PluginManager::updateDockOverflowState(int state)
{
    QJsonObject obj;
    obj[dock::MSG_TYPE] = dock::MSG_UPDATE_OVERFLOW_STATE;
    obj[dock::MSG_DATA] = state;

    send_event_message(toJson(obj));
}

void PluginManager::dockPanelSizeChanged(int width, int height)
{
    QJsonObject sizeData;
    sizeData["width"] = width;
    sizeData["height"] = height;

    QJsonObject obj;
    obj[dock::MSG_TYPE] = dock::MSG_DOCK_PANEL_SIZE_CHANGED;
    obj[dock::MSG_DATA] = sizeData;

    send_event_message(toJson(obj));
}

uint32_t PluginManager::dockPosition() const
{
    return m_dockPosition;
}

void PluginManager::setDockPosition(uint32_t dockPosition)
{
    if (m_dockPosition == dockPosition)
        return;

    m_dockPosition = dockPosition;
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        if (target) {
            send_position_changed(target->handle, m_dockPosition);
        }
    }
}

uint32_t PluginManager::dockColorTheme() const
{
    return m_dockColorTheme;
}

void PluginManager::setDockColorTheme(uint32_t type)
{
    if (type == m_dockColorTheme)
        return;

    m_dockColorTheme = type;
    foreach (PluginSurface *plugin, m_pluginSurfaces) {
        Resource *target = resourceMap().value(plugin->surface()->waylandClient());
        if (target) {
            send_color_theme_changed(target->handle, m_dockColorTheme);
        }
    }
}

void PluginManager::plugin_manager_v1_request_message(Resource *resource, const QString &plugin_id, const QString &item_key, const QString &msg)
{
    qInfo() << "server pluginManager receive client:" << plugin_id << item_key << " msg:" << msg;
    PluginSurface *dstPlugin = nullptr;
    for (PluginSurface *plugin : m_pluginSurfaces) {
        if (plugin->pluginId() == plugin_id && plugin->itemKey() == item_key) {
            dstPlugin = plugin;
            break;
        }
    }

    if (!dstPlugin) {
        return;
    }

    auto rootObj = getRootObj(msg);
    if (rootObj.isEmpty()) {
        return;
    }

    const auto &msgType = rootObj.value(dock::MSG_TYPE);
    if (msgType == dock::MSG_SUPPORT_FLAG_CHANGED) {
        // todo
    } else if (msgType == dock::MSG_ITEM_ACTIVE_STATE) {
        dstPlugin->setItemActive(rootObj.value(dock::MSG_DATA).toBool());
    } else if (msgType == dock::MSG_UPDATE_TOOLTIPS_VISIBLE) {

    }
}

void PluginManager::plugin_manager_v1_create_plugin(Resource *resource, const QString &pluginId, const QString &itemKey, int32_t plugin_flags, int32_t type, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    connect(qwaylandSurface, &QWaylandSurface::surfaceDestroyed, this, [this, qwaylandSurface](){
        for (PluginSurface *plugin : m_pluginSurfaces) {
            if (plugin->surface() == qwaylandSurface) {
                Q_EMIT pluginSurfaceDestroyed(plugin);
                m_pluginSurfaces.removeAll(plugin);
                break;
            }
        }
    });

    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_interface,
                                                           wl_resource_get_version(resource->handle), id));

    send_position_changed(resource->handle, m_dockPosition);
    send_color_theme_changed(resource->handle, m_dockColorTheme);

    auto plugin = new PluginSurface(this, pluginId, itemKey, plugin_flags, type, qwaylandSurface, shellSurfaceResource);
    m_pluginSurfaces << plugin;
    Q_EMIT pluginSurfaceCreated(plugin);
}

void PluginManager::plugin_manager_v1_create_popup_at(Resource *resource, int32_t x, int32_t y, int32_t type, struct ::wl_resource *surface, uint32_t id)
{
    QWaylandSurface *qwaylandSurface = QWaylandSurface::fromResource(surface);
    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &::plugin_popup_interface,
                                                           wl_resource_get_version(resource->handle), id));

    auto plugin = new PluginPopup(this, x, y, type, qwaylandSurface, shellSurfaceResource);
    plugin->setX(x), plugin->setY(y);
    Q_EMIT pluginPopupCreated(plugin);
}

QJsonObject PluginManager::getRootObj(const QString &jsonStr) {
    QJsonParseError jsonParseError;
    const QJsonDocument &resultDoc = QJsonDocument::fromJson(jsonStr.toLocal8Bit(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError || resultDoc.isEmpty()) {
        qWarning() << "Result json parse error";
        return QJsonObject();
    }

    return resultDoc.object();
}

QString PluginManager::toJson(const QJsonObject &jsonObj)
{
    QJsonDocument doc;
    doc.setObject(jsonObj);
    return doc.toJson();
}