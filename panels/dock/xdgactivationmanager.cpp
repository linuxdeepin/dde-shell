// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xdgactivationmanager_p.h"

#include <wayland/xdgactivation.h>

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QWindow>

#include <QtWaylandCompositor/QWaylandSeat>

Q_LOGGING_CATEGORY(xdgActivationMgr, "dde.shell.xdgactivation.manager")

// ---------------------------------------------------------------------------
// XdgActivationManager
// ---------------------------------------------------------------------------

XdgActivationManager::XdgActivationManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
    , m_compositor(compositor)
{
}

void XdgActivationManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    Q_ASSERT(compositor);
    m_compositor = compositor;
    init(compositor->display(), 1);

    // Create the client-side xdg_activation_v1 connection to the outer compositor
    m_outerActivation = new ds::XdgActivation(this);
    connect(m_outerActivation, &ds::XdgActivation::tokenReady, this, &XdgActivationManager::onTokenReady);
}

void XdgActivationManager::xdg_activation_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void XdgActivationManager::xdg_activation_v1_get_activation_token(Resource *resource, uint32_t id)
{
    QWaylandResource tokenResource(wl_resource_create(resource->client(), &xdg_activation_token_v1_interface, wl_resource_get_version(resource->handle), id));
    new XdgActivationTokenV1(this, tokenResource);
}

void XdgActivationManager::setPendingToken(XdgActivationTokenV1 *token)
{
    m_pendingToken = token;
}

void XdgActivationManager::requestOuterToken(const QString &appId)
{
    QWindow *window = QGuiApplication::focusWindow();
    m_outerActivation->requestToken(window, appId);
}

void XdgActivationManager::clearPendingTokenIf(XdgActivationTokenV1 *token)
{
    if (m_pendingToken == token) {
        m_pendingToken = nullptr;
    }
}

void XdgActivationManager::onTokenReady(const QString &token)
{
    if (m_pendingToken) {
        qCDebug(xdgActivationMgr) << "Forwarding activation token to plugin client";
        m_pendingToken->sendToken(token);
        m_pendingToken = nullptr;
    }
}

// ---------------------------------------------------------------------------
// XdgActivationTokenV1
// ---------------------------------------------------------------------------
// XdgActivationTokenV1
// ---------------------------------------------------------------------------

XdgActivationTokenV1::XdgActivationTokenV1(XdgActivationManager *manager, const QWaylandResource &resource)
    : QObject(manager)
    , m_manager(manager)
{
    init(resource.resource());
}

XdgActivationTokenV1::~XdgActivationTokenV1() = default;

void XdgActivationTokenV1::sendToken(const QString &token)
{
    send_done(token);
}

void XdgActivationTokenV1::xdg_activation_token_v1_set_serial(Resource *resource, uint32_t serial, struct ::wl_resource *seat)
{
    Q_UNUSED(resource)
    Q_UNUSED(seat)
    m_serial = serial;
}

void XdgActivationTokenV1::xdg_activation_token_v1_set_app_id(Resource *resource, const QString &app_id)
{
    Q_UNUSED(resource)
    m_appId = app_id;
}

void XdgActivationTokenV1::xdg_activation_token_v1_set_surface(Resource *resource, struct ::wl_resource *surface)
{
    Q_UNUSED(resource)
    m_surface = QWaylandSurface::fromResource(surface);
}

void XdgActivationTokenV1::xdg_activation_token_v1_commit(Resource *resource)
{
    Q_UNUSED(resource)
    qCDebug(xdgActivationMgr) << "Token committed by plugin client, appId:" << m_appId;

    // Store this token as pending and request from the outer compositor
    m_manager->setPendingToken(this);
    m_manager->requestOuterToken(m_appId);
}

void XdgActivationTokenV1::xdg_activation_token_v1_destroy(Resource *resource)
{
    Q_UNUSED(resource)
    m_manager->clearPendingTokenIf(this);
    deleteLater();
}
