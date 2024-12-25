// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "dlayershellwindow.h"

#include <private/qwaylandwindow_p.h>
#include <qwayland-wlr-layer-shell-unstable-v1.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include "qwayland-treeland-dde-shell-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>

DS_BEGIN_NAMESPACE
class DDEShellSurface;
class QWaylandLayerShellSurface : public QtWaylandClient::QWaylandShellSurface, public QtWayland::zwlr_layer_surface_v1
{
    Q_OBJECT
public:
    QWaylandLayerShellSurface(QtWayland::zwlr_layer_shell_v1 *shell, QtWaylandClient::QWaylandWindow *window);
    ~QWaylandLayerShellSurface() override;

    bool isExposed() const override
    {
        return m_configured;
    }

    void applyConfigure() override;
    void setWindowGeometry(const QRect &geometry) override;
    void attachPopup(QWaylandShellSurface *popup) override;
    bool resize(QtWaylandClient::QWaylandInputDevice *, Qt::Edges) override;

private:
    void zwlr_layer_surface_v1_configure(uint32_t serial, uint32_t width, uint32_t height) override;
    void zwlr_layer_surface_v1_closed() override;

    void calcAndSetRequestSize(QSize requestSize);
    bool anchorsSizeConflict() const;
    void trySetAnchorsAndSize();

    struct ::wl_surface *m_surface;
    QScopedPointer<DDEShellSurface> m_ddeShellSurface;
    DLayerShellWindow* m_dlayerShellWindow;
    QSize m_pendingSize;
    QSize m_requestSize;
    bool m_configured = false;
};

class TreeLandDDEShellManager : public QWaylandClientExtensionTemplate<TreeLandDDEShellManager>, public QtWayland::treeland_dde_shell_manager_v1
{
    Q_OBJECT

private:
    explicit TreeLandDDEShellManager();

public:
    static TreeLandDDEShellManager *instance();
    DDEShellSurface *getDDEShellSurface(struct ::wl_surface *surface);
};

class DDEShellSurface : public QWaylandClientExtensionTemplate<DDEShellSurface>, public QtWayland::treeland_dde_shell_surface_v1
{
    Q_OBJECT

public:
    explicit DDEShellSurface(struct ::treeland_dde_shell_surface_v1 *object);
    ~DDEShellSurface() override;
};

DS_END_NAMESPACE
