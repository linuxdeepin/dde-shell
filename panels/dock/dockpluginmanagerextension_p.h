// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtWaylandCompositor/QWaylandShellSurfaceTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>

#include "qwayland-server-dock-plugin-manager-v1.h"

class PluginSurface;
class DockPluginManager : public QWaylandCompositorExtensionTemplate<DockPluginManager>, public QtWaylandServer::dock_plugin_manager_v1
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(uint32_t dockPosition READ dockPosition WRITE setDockPosition)
    Q_PROPERTY(uint32_t dockDisplayMode READ dockDisplayMode WRITE setDockDisplayMode)
    Q_PROPERTY(uint32_t dockColorTheme READ dockColorTheme WRITE setDockColorTheme)

public:
    enum SurfaceType {
        Tooltip = surface_type_tooltip,
        Popup = surface_type_popup,
        Tray = surface_type_tray,
        Fixed = surface_type_fixed,
        System = surface_type_system,
        Tool = surface_type_tool,
        Quick = surface_type_qucik,
        SlidingPanel = surface_type_panel
    };
    Q_ENUM(SurfaceType)

    DockPluginManager(QWaylandCompositor *compositor = nullptr);
    void initialize() override;

    uint32_t dockPosition() const;
    void setDockPosition(uint32_t dockPosition);

    uint32_t dockDisplayMode() const;
    void setDockDisplayMode(uint32_t dockDisplayMode);

    uint32_t dockColorTheme() const;
    void setDockColorTheme(uint32_t type);

Q_SIGNALS:
    void pluginSurfaceCreated(PluginSurface*);
    void pluginSurfaceDestroyed(PluginSurface*);

protected:
    void dock_plugin_manager_v1_create_plugin_surface(Resource *resource, const QString &pluginId, const QString &itemKey, uint32_t surfaceType, struct ::wl_resource *surface, uint32_t id) override;

private:
    QList<QWaylandSurface*> m_pluginSurfaces;
    uint32_t m_dockPosition;
    uint32_t m_dockColorTheme;
    uint32_t m_dockDisplayMode;
};

class PluginSurface : public QWaylandShellSurfaceTemplate<PluginSurface>, public QtWaylandServer::dock_plugin_surface
{
    Q_OBJECT
    Q_PROPERTY(QString pluginId READ pluginId)
    Q_PROPERTY(QString itemKey READ itemKey)
    Q_PROPERTY(QString contextMenu READ contextMenu NOTIFY contextMenuChanged)
    Q_PROPERTY(int32_t pluginFlags READ pluginFlags)
    Q_PROPERTY(DockPluginManager::SurfaceType surfaceType READ surfaceType)

public:
    PluginSurface(DockPluginManager* shell, const QString& pluginId, const QString& itemKey, DockPluginManager::SurfaceType surfaceType, QWaylandSurface *surface, const QWaylandResource &resource);
    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const;

    QString pluginId() const;
    QString itemKey() const;
    QString contextMenu() const;
    int32_t pluginFlags() const;
    DockPluginManager::SurfaceType surfaceType() const;
    Q_INVOKABLE void click(const QString &menuId, uint32_t checked);

Q_SIGNALS:
    void contextMenuChanged();

protected:
    void dock_plugin_surface_request_set_applet_visible(Resource *resource, const QString &itemKey, uint32_t visible) override;
    void dock_plugin_surface_create_context_menu(Resource *resource, const QString &contextMenu) override;
    void dock_plugin_surface_create_dcc_icon(Resource *resource, const QString &dccIcon) override;
    void dock_plugin_surface_plugin_flags(Resource *resource, int32_t flags) override;

private:
    QWaylandSurface* m_surface;
    DockPluginManager* m_manager;

    QString m_itemKey;
    QString m_pluginId;

    QString m_dccIcon;
    QString m_contextMenu;
    int32_t m_flags;

    DockPluginManager::SurfaceType m_surfaceType;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(DockPluginManager)
