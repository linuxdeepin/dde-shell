// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtWaylandCompositor/QWaylandShellSurfaceTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <cstdint>

#include "qwayland-server-plugin-manager-v1.h"

class PluginSurface;
class PluginPopup;
class PluginManager : public QWaylandCompositorExtensionTemplate<PluginManager>, public QtWaylandServer::plugin_manager_v1
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(uint32_t dockPosition READ dockPosition WRITE setDockPosition)
    Q_PROPERTY(uint32_t dockColorTheme READ dockColorTheme WRITE setDockColorTheme)

public:
    PluginManager(QWaylandCompositor *compositor = nullptr);
    void initialize() override;

    Q_INVOKABLE void updateDockOverflowState(int state);
    Q_INVOKABLE void dockPanelSizeChanged(int width, int height);

    uint32_t dockPosition() const;
    void setDockPosition(uint32_t dockPosition);

    uint32_t dockColorTheme() const;
    void setDockColorTheme(uint32_t type);

Q_SIGNALS:
    void pluginPopupCreated(PluginPopup*);
    void pluginSurfaceCreated(PluginSurface*);
    void pluginSurfaceDestroyed(PluginSurface*);
    void messageRequest(PluginSurface *, const QString &msg);

protected:
    virtual void plugin_manager_v1_request_message(Resource *resource, const QString &plugin_id, const QString &item_key, const QString &msg) override;
    virtual void plugin_manager_v1_create_popup_at(Resource *resource, const QString &plugin_id, const QString &item_key, int32_t type, int32_t x, int32_t y, struct ::wl_resource *surface, uint32_t id) override;
    virtual void plugin_manager_v1_create_plugin(Resource *resource, const QString &plugin_id, const QString &item_key, int32_t plugin_flags, int32_t type, struct ::wl_resource *surface, uint32_t id) override;

private:
    static QJsonObject getRootObj(const QString &jsonStr);
    static QString toJson(const QJsonObject &jsonObj);

private:
    QList<PluginSurface*> m_pluginSurfaces;

    uint32_t m_dockPosition;
    uint32_t m_dockColorTheme;
};

class PluginSurface : public QWaylandShellSurfaceTemplate<PluginSurface>, public QtWaylandServer::plugin
{
    Q_OBJECT
    Q_PROPERTY(QString pluginId READ pluginId)
    Q_PROPERTY(QString itemKey READ itemKey)
    Q_PROPERTY(uint32_t pluginFlags READ pluginFlags)
    Q_PROPERTY(uint32_t pluginType READ pluginType)
    Q_PROPERTY(bool isItemActive WRITE setItemActive READ isItemActive NOTIFY itemActiveChanged)

public:
    PluginSurface(PluginManager* shell, const QString& pluginId, const QString& itemKey, int pluginFlags, int pluginType, QWaylandSurface *surface, const QWaylandResource &resource);
    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const;

    QString pluginId() const;
    QString itemKey() const;
    QString contextMenu() const;
    uint32_t pluginType() const;
    uint32_t pluginFlags() const;

    void setItemActive(bool isActive);
    bool isItemActive() const;

    Q_INVOKABLE void updatePluginGeometry(const QRect &geometry);

signals:
    void itemActiveChanged();

private:
    PluginManager* m_manager;
    QWaylandSurface* m_surface;

    QString m_itemKey;
    QString m_pluginId;

    uint32_t m_flags;
    uint32_t m_pluginType;

    bool m_isItemActive = false;
};

class PluginPopup : public QWaylandShellSurfaceTemplate<PluginSurface>, public QtWaylandServer::plugin_popup
{
    Q_OBJECT
    Q_PROPERTY(int32_t x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int32_t y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(QString pluginId READ pluginId)
    Q_PROPERTY(QString itemKey READ itemKey)
    Q_PROPERTY(int32_t popupType READ popupType)

public:
    PluginPopup(PluginManager* shell, const QString &pluginId, const QString &itemKey, int x, int y, int popupType,
                QWaylandSurface *surface, const QWaylandResource &resource);
    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const;

    Q_INVOKABLE void close() { send_close(); }
    Q_INVOKABLE void updatePluginGeometry(const QRect &geometry);

    QString pluginId() const;
    QString itemKey() const;

    int32_t x() const;
    int32_t y() const;

    void setX(int32_t x);
    void setY(int32_t y);

    int32_t popupType() const;

Q_SIGNALS:
    void xChanged();
    void yChanged();

private:
    PluginManager* m_manager;
    QWaylandSurface* m_surface;

    QString m_itemKey;
    QString m_pluginId;
    int32_t m_popupType;

    int32_t m_x;
    int32_t m_y;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(PluginManager)
