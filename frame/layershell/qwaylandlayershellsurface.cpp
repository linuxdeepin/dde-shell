// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
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

Q_LOGGING_CATEGORY(layershellsurface, "org.deepin.dde.shell.layershell.surface")

DS_BEGIN_NAMESPACE

QWaylandLayerShellSurface::QWaylandLayerShellSurface(QtWayland::zwlr_layer_shell_v1 *shell, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::zwlr_layer_surface_v1()
    , m_dlayerShellWindow(DLayerShellWindow::get(window->window()))
{
    wl_output *output = nullptr;
    if (m_dlayerShellWindow->screenConfiguration() == DLayerShellWindow::ScreenFromQWindow) {
        output = currentOutput();
        connect(window->window(), &QWindow::screenChanged, this, &QWaylandLayerShellSurface::scheduleRecreate);
        if (!output) {
            qCWarning(layershellsurface) << "failed to get screen for wayland";
        }
    }

    init(shell->get_layer_surface(window->waylandSurface()->object(), output, m_dlayerShellWindow->layer(), m_dlayerShellWindow->scope()));
    m_output = output;

    applyLayer();
    connect(m_dlayerShellWindow, &DLayerShellWindow::layerChanged, this, &QWaylandLayerShellSurface::applyLayer);

    set_anchor(m_dlayerShellWindow->anchors());
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, this, &QWaylandLayerShellSurface::trySetAnchorsAndSize);

    applyExclusiveZone();
    connect(m_dlayerShellWindow, &DLayerShellWindow::exclusionZoneChanged, this, &QWaylandLayerShellSurface::applyExclusiveZone);

    applyMargins();
    connect(m_dlayerShellWindow, &DLayerShellWindow::marginsChanged, this, &QWaylandLayerShellSurface::applyMargins);

    applyKeyboardInteractivity();
    connect(m_dlayerShellWindow, &DLayerShellWindow::keyboardInteractivityChanged, this, &QWaylandLayerShellSurface::applyKeyboardInteractivity);

    applyInputRegion();
    connect(m_dlayerShellWindow, &DLayerShellWindow::inputRegionChanged, this, &QWaylandLayerShellSurface::applyInputRegion);

    calcAndSetRequestSize(window->surfaceSize());

    if (m_requestSize.isValid()) {
        set_size(m_requestSize.width(), m_requestSize.height());
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

QtWaylandClient::QWaylandWindow *QWaylandLayerShellSurface::waylandWindow()
{
    return window();
}

QWindow *QWaylandLayerShellSurface::windowHandle()
{
    auto currentWindow = waylandWindow();
    return currentWindow ? currentWindow->window() : nullptr;
}

QtWaylandClient::QWaylandScreen *QWaylandLayerShellSurface::waylandScreen()
{
    auto currentWindowHandle = windowHandle();
    if (!currentWindowHandle || !currentWindowHandle->screen()) {
        return nullptr;
    }

    return dynamic_cast<QtWaylandClient::QWaylandScreen *>(currentWindowHandle->screen()->handle());
}

wl_output *QWaylandLayerShellSurface::currentOutput()
{
    auto currentWaylandScreen = waylandScreen();
    return currentWaylandScreen ? currentWaylandScreen->output() : nullptr;
}

void QWaylandLayerShellSurface::calcAndSetRequestSize(QSize requestSize)
{
    auto anchors = m_dlayerShellWindow->anchors();
    const bool horizontallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorLeft, DLayerShellWindow::AnchorRight});
    const bool verticallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorTop, DLayerShellWindow::AnchorBottom});
    m_requestSize = requestSize;
    if (horizontallyConstrained) {
        m_requestSize.setWidth(0);
    }
    if (verticallyConstrained) {
        m_requestSize.setHeight(0);
    }
}

bool QWaylandLayerShellSurface::anchorsSizeConflict() const
{
    auto anchors = m_dlayerShellWindow->anchors();
    const bool horizontallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorLeft, DLayerShellWindow::AnchorRight});
    const bool verticallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorTop, DLayerShellWindow::AnchorBottom});
    return (!horizontallyConstrained && m_requestSize.width() == 0) || (!verticallyConstrained && m_requestSize.height() == 0);
}

void QWaylandLayerShellSurface::trySetAnchorsAndSize()
{
    if (!anchorsSizeConflict()) {
        set_anchor(m_dlayerShellWindow->anchors());
        set_size(m_requestSize.width(), m_requestSize.height());
        scheduleCommit();
    }
}

void QWaylandLayerShellSurface::applyLayer()
{
    set_layer(m_dlayerShellWindow->layer());
    scheduleCommit();
}

void QWaylandLayerShellSurface::applyExclusiveZone()
{
    set_exclusive_zone(m_dlayerShellWindow->exclusionZone());
    scheduleCommit();
}

void QWaylandLayerShellSurface::applyMargins()
{
    set_margin(m_dlayerShellWindow->topMargin(), m_dlayerShellWindow->rightMargin(), m_dlayerShellWindow->bottomMargin(), m_dlayerShellWindow->leftMargin());
    scheduleCommit();
}

void QWaylandLayerShellSurface::applyKeyboardInteractivity()
{
    set_keyboard_interactivity(m_dlayerShellWindow->keyboardInteractivity());
    scheduleCommit();
}

void QWaylandLayerShellSurface::applyInputRegion()
{
    auto currentWindowHandle = windowHandle();
    if (!currentWindowHandle) {
        return;
    }

    currentWindowHandle->setMask(m_dlayerShellWindow->inputRegion());
    scheduleCommit();
}

void QWaylandLayerShellSurface::zwlr_layer_surface_v1_configure(uint32_t serial, uint32_t width, uint32_t height)
{
    ack_configure(serial);
    m_pendingSize = QSize(width, height);

    if (!m_configured) {
        m_configured = true;
        window()->resizeFromApplyConfigure(m_pendingSize);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        window()->handleExpose(QRect(QPoint(), m_pendingSize));
#elif QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
        window()->sendRecursiveExposeEvent();
#else
        window()->updateExposure();
#endif

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
    calcAndSetRequestSize(geometry.size());
    trySetAnchorsAndSize();
}

void QWaylandLayerShellSurface::commitWindowState()
{
    auto currentWaylandWindow = waylandWindow();
    if (!currentWaylandWindow || !currentWaylandWindow->waylandSurface()) {
        return;
    }

    currentWaylandWindow->waylandSurface()->commit();
}

void QWaylandLayerShellSurface::flushCommit()
{
    m_commitScheduled = false;
    commitWindowState();
}

void QWaylandLayerShellSurface::scheduleCommit()
{
    if (m_commitScheduled) {
        return;
    }

    m_commitScheduled = true;
    QMetaObject::invokeMethod(this, &QWaylandLayerShellSurface::flushCommit, Qt::QueuedConnection);
}

void QWaylandLayerShellSurface::recreateWindow()
{
    auto currentWaylandWindow = waylandWindow();
    if (!currentWaylandWindow) {
        return;
    }

    currentWaylandWindow->reset();
    QMetaObject::invokeMethod(currentWaylandWindow, &QtWaylandClient::QWaylandWindow::reinit, Qt::QueuedConnection);
}

void QWaylandLayerShellSurface::flushRecreate()
{
    m_recreateScheduled = false;
    recreateWindow();
}

void QWaylandLayerShellSurface::scheduleRecreate()
{
    auto currentWindowHandle = windowHandle();
    if (!currentWindowHandle) {
        return;
    }

    const auto output = currentOutput();
    if (!output) {
        qCWarning(layershellsurface) << "failed to get screen for wayland";
        return;
    }

    if (output == m_output || m_recreateScheduled) {
        return;
    }

    m_output = output;
    m_recreateScheduled = true;
    QMetaObject::invokeMethod(this, &QWaylandLayerShellSurface::flushRecreate, Qt::QueuedConnection);
}

void QWaylandLayerShellSurface::attachPopup(QtWaylandClient::QWaylandShellSurface *popup)
{
    std::any anyRole = popup->surfaceRole();

    if (auto role = std::any_cast<::xdg_popup *>(&anyRole)) {
        get_popup(*role);
    } else {
        qCWarning(layershellsurface) << "Cannot attach popup of unknown type";
    }
}

DS_END_NAMESPACE
