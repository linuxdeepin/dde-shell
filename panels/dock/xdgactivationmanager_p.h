// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QPointer>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandCompositorExtension>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandSurface>

#include "qwayland-server-xdg-activation-v1.h"

namespace ds
{
class XdgActivation;
}

class XdgActivationTokenV1;
class XdgActivationManager : public QWaylandCompositorExtensionTemplate<XdgActivationManager>, public QtWaylandServer::xdg_activation_v1
{
    Q_OBJECT
    QML_ELEMENT
public:
    XdgActivationManager(QWaylandCompositor *compositor = nullptr);
    void initialize() override;

    void setPendingToken(XdgActivationTokenV1 *token);
    void requestOuterToken(const QString &appId);
    void clearPendingTokenIf(XdgActivationTokenV1 *token);

protected:
    void xdg_activation_v1_destroy(Resource *resource) override;
    void xdg_activation_v1_get_activation_token(Resource *resource, uint32_t id) override;

private:
    void onTokenReady(const QString &token);

    QWaylandCompositor *m_compositor = nullptr;
    ds::XdgActivation *m_outerActivation = nullptr;
    QPointer<XdgActivationTokenV1> m_pendingToken;
};

class XdgActivationTokenV1 : public QObject, public QtWaylandServer::xdg_activation_token_v1
{
    Q_OBJECT
public:
    XdgActivationTokenV1(XdgActivationManager *manager, const QWaylandResource &resource);
    ~XdgActivationTokenV1() override;

    void sendToken(const QString &token);

protected:
    void xdg_activation_token_v1_set_serial(Resource *resource, uint32_t serial, struct ::wl_resource *seat) override;
    void xdg_activation_token_v1_set_app_id(Resource *resource, const QString &app_id) override;
    void xdg_activation_token_v1_set_surface(Resource *resource, struct ::wl_resource *surface) override;
    void xdg_activation_token_v1_commit(Resource *resource) override;
    void xdg_activation_token_v1_destroy(Resource *resource) override;

private:
    XdgActivationManager *m_manager;
    QPointer<QWaylandSurface> m_surface;
    uint32_t m_serial = 0;
    QString m_appId;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(XdgActivationManager)
