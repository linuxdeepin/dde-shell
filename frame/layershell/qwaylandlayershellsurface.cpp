// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "dlayershellwindow.h"
#include "qwaylandlayershellsurface_p.h"

#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <wayland-wlr-layer-shell-unstable-v1-client-protocol.h>

#include <QLoggingCategory>

#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

Q_LOGGING_CATEGORY(layershellsurface, "dde.shell.layershell.surface")

DS_BEGIN_NAMESPACE

QWaylandLayerShellSurface::QWaylandLayerShellSurface(QtWayland::zwlr_layer_shell_v1 *shell, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::zwlr_layer_surface_v1()
    , m_dlayerShellWindow(DLayerShellWindow::get(window->window()))
{

    wl_output *output = nullptr;
    if (m_dlayerShellWindow->screenConfiguration() == DLayerShellWindow::ScreenFromQWindow) {
        auto waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(window->window()->screen()->handle());
        if (!waylandScreen) {
            qCWarning(layershellsurface) << "failed to get screen for wayland";
        } else {
            output = waylandScreen->output();
        }
    }

    init(shell->get_layer_surface(window->waylandSurface()->object(), output, m_dlayerShellWindow->layer(), m_dlayerShellWindow->scope()));

    set_layer(m_dlayerShellWindow->layer());
    connect(m_dlayerShellWindow, &DLayerShellWindow::layerChanged, this, [this, window](){
        set_layer(m_dlayerShellWindow->layer());
        window->waylandSurface()->commit();
    });

    set_anchor(m_dlayerShellWindow->anchors());
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, this,[this, window](){
        set_anchor(m_dlayerShellWindow->anchors());
        window->waylandSurface()->commit();
    });

    set_exclusive_zone(m_dlayerShellWindow->exclusionZone());
    connect(m_dlayerShellWindow, &DLayerShellWindow::exclusionZoneChanged, this,[this, window](){
        set_exclusive_zone(m_dlayerShellWindow->exclusionZone());
        window->waylandSurface()->commit();
    });

    set_margin(m_dlayerShellWindow->topMargin(), m_dlayerShellWindow->rightMargin(), m_dlayerShellWindow->bottomMargin(), m_dlayerShellWindow->leftMargin());
    connect(m_dlayerShellWindow, &DLayerShellWindow::marginsChanged, this, [this](){
        set_margin(m_dlayerShellWindow->topMargin(), m_dlayerShellWindow->rightMargin(), m_dlayerShellWindow->bottomMargin(), m_dlayerShellWindow->leftMargin());
    });

    set_keyboard_interactivity(m_dlayerShellWindow->keyboardInteractivity());
    connect(m_dlayerShellWindow, &DLayerShellWindow::keyboardInteractivityChanged, this, [this, window](){
        set_keyboard_interactivity(m_dlayerShellWindow->keyboardInteractivity());
        window->waylandSurface()->commit();
    });


    QSize size = window->surfaceSize();
    const DLayerShellWindow::Anchors anchors = m_dlayerShellWindow->anchors();
    if ((anchors & DLayerShellWindow::AnchorLeft) && (anchors & DLayerShellWindow::AnchorRight)) {
        size.setWidth(0);
    }

    if ((anchors & DLayerShellWindow::AnchorTop) && (anchors & DLayerShellWindow::AnchorBottom )) {
        size.setHeight(0);
    }

    if (size.isValid()) {
        set_size(size.width(), size.height());
    }
}

QWaylandLayerShellSurface::~QWaylandLayerShellSurface()
{
    destroy();
}

void QWaylandLayerShellSurface::zwlr_layer_surface_v1_closed()
{
    if (m_dlayerShellWindow->closeOnDismissed()) {
        window()->window()->close();
    }
}

void QWaylandLayerShellSurface::zwlr_layer_surface_v1_configure(uint32_t serial, uint32_t width, uint32_t height)
{
    ack_configure(serial);
    m_pendingSize = QSize(width, height);

    if (!m_configured) {
        m_configured = true;
        window()->resizeFromApplyConfigure(m_pendingSize);
        window()->handleExpose(QRect(QPoint(), m_pendingSize));
    } else {
        window()->applyConfigureWhenPossible();
    }
}

void QWaylandLayerShellSurface::applyConfigure()
{
    window()->resizeFromApplyConfigure(m_pendingSize);
}

void QWaylandLayerShellSurface::setWindowGeometry(const QRect &geometry)
{
    auto anchors = m_dlayerShellWindow->anchors();
    const bool horizontallyConstrained =(anchors & (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight)) == (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight);
    const bool verticallyConstrained = (anchors & (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom)) == (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom);

    QSize size = geometry.size();
    if (horizontallyConstrained) {
        size.setWidth(0);
    }
    if (verticallyConstrained) {
        size.setHeight(0);
    }
    set_size(size.width(), size.height());
}

DS_END_NAMESPACE
