// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "dlayershellwindow.h"
#include "qwaylandlayershellsurface_p.h"
#include "layershellextension_p.h"

#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <wayland-wlr-layer-shell-unstable-v1-client-protocol.h>

#include <QLoggingCategory>
#include <QScopedValueRollback>
#include <private/qhighdpiscaling_p.h>

#include <QtWaylandClient/private/qwaylandinputdevice_p.h>
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
    m_shellExtension.reset();
    if (m_dlayerShellWindow)
        m_dlayerShellWindow->setResizing(false);
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
        applyConfigure();
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
    if (m_interactiveResize) {
        // Keep layer layout in step with the size Qt is about to render. In
        // particular, margin/exclusive-zone commits must not re-anchor the
        // surface using its pre-drag size. Do not echo unchanged replies.
        const QSize previousRequest = m_requestSize;
        calcAndSetRequestSize(m_pendingSize);
        if (m_requestSize != previousRequest)
            trySetAnchorsAndSize();
    }

    QScopedValueRollback<bool> applyingConfigure(m_applyingConfigure, true);
    window()->resizeFromApplyConfigure(m_pendingSize);
}

void QWaylandLayerShellSurface::finishResize()
{
    // An interactive resize can end before Qt applies its last configure.
    // Adopt it before clients persist their size or restore geometry bindings.
    if (m_interactiveResize && m_configured) {
        calcAndSetRequestSize(m_pendingSize);
        trySetAnchorsAndSize();
        Q_EMIT m_dlayerShellWindow->resizeFinished(QHighDpi::fromNativePixels(m_pendingSize, windowHandle()));
    }
    m_interactiveResize = false;
    m_dlayerShellWindow->setResizing(false);
}

bool QWaylandLayerShellSurface::resize(QtWaylandClient::QWaylandInputDevice *inputDevice, Qt::Edges edges)
{
    if (!inputDevice || !inputDevice->wl_seat() || !m_configured || m_dlayerShellWindow->isResizing())
        return false;

    auto *manager = TreeLandLayerShellExtensionManager::instance();
    if (!manager->isActive())
        return false;

    // Qt::Edge and the protocol's xdg_toplevel-compatible enum differ.
    uint32_t protocolEdges = 0;
    if (edges.testFlag(Qt::TopEdge))
        protocolEdges |= TreeLandLayerShellExtensionObject::resize_edge_top;
    if (edges.testFlag(Qt::BottomEdge))
        protocolEdges |= TreeLandLayerShellExtensionObject::resize_edge_bottom;
    if (edges.testFlag(Qt::LeftEdge))
        protocolEdges |= TreeLandLayerShellExtensionObject::resize_edge_left;
    if (edges.testFlag(Qt::RightEdge))
        protocolEdges |= TreeLandLayerShellExtensionObject::resize_edge_right;

    if (!m_shellExtension) {
        m_shellExtension.reset(new TreeLandLayerShellExtensionObject(
            manager->get_layer_shell_extension_object(window()->wlSurface())));
        connect(m_shellExtension.get(), &TreeLandLayerShellExtensionObject::resizingChanged, this, [this](bool resizing) {
            if (resizing)
                m_interactiveResize = true;
            else
                finishResize();
        });
        connect(m_shellExtension.get(), &TreeLandLayerShellExtensionObject::resizeRejected, this, [this](uint32_t error) {
            qCWarning(layershellsurface) << "Layer surface resize rejected:" << error;
            finishResize();
        });
    }

    const QSize minSize = window()->windowMinimumSize();
    const QSize maxSize = window()->windowMaximumSize();
    m_shellExtension->begin_resize(inputDevice->wl_seat(), inputDevice->serial(), protocolEdges,
                                  qMax(0, minSize.width()), qMax(0, minSize.height()),
                                  maxSize.width() == QWINDOWSIZE_MAX ? 0 : qMax(0, maxSize.width()),
                                  maxSize.height() == QWINDOWSIZE_MAX ? 0 : qMax(0, maxSize.height()));
    // QWaylandWindow releases the client pointer grab after this returns.
    // Keep clients in resize mode while waiting for acceptance or rejection.
    m_dlayerShellWindow->setResizing(true);
    return true;
}

void QWaylandLayerShellSurface::setWindowGeometry(const QRect &geometry)
{
    // Qt also calls this while applying a compositor configure. Echoing that
    // size back can replace a newer client request and start a resize loop.
    if (m_applyingConfigure)
        return;

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
