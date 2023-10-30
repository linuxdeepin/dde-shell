// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dlayershellwindow.h"
#include "x11dlayershellemulation.h"

#include <QDebug>
#include <QScreen>
#include <QMargins>
#include <QGuiApplication>

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

LayerShellEmulation::LayerShellEmulation(QWindow* window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_dlayerShellWindow(DS_NAMESPACE::DLayerShellWindow::get(m_window))
{
    m_window->setFlag(Qt::FramelessWindowHint);
    onLayerChanged();
    connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::layerChanged, this, &LayerShellEmulation::onLayerChanged);

    onPositionChanged();
    connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::anchorsChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::marginsChanged, this, &LayerShellEmulation::onPositionChanged);

    onExclusionZoneChanged();
    connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::anchorsChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::exclusionZoneChanged, this, &LayerShellEmulation::onExclusionZoneChanged);

    // qml height or wdth may update later, need to update anchor postion and exclusion zone
    connect(m_window, &QWindow::widthChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_window, &QWindow::widthChanged, this, &LayerShellEmulation::onPositionChanged);

    connect(m_window, &QWindow::heightChanged, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_window, &QWindow::heightChanged, this, &LayerShellEmulation::onPositionChanged);

    // connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::keyboardInteractivityChanged, this, &LayerShellEmulation::onKeyboardInteractivityChanged);
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#STACKINGORDER
  * the following layered stacking order is recommended, from the bottom
  * _NET_WM_TYPE_DESKTOP
  * _NET_WM_STATE_BELOW
  * windows not belonging in any other layer
  * _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and windows having state _NET_WM_STATE_ABOVE
  * focused windows having state _NET_WM_STATE_FULLSCREEN
  */
void LayerShellEmulation::onLayerChanged()
{
    auto xcbWindow = dynamic_cast<QNativeInterface::Private::QXcbWindow*>(m_window->handle());
    switch (m_dlayerShellWindow->layer()) {
        case DS_NAMESPACE::DLayerShellWindow::LayerBackground: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Desktop);
            break;
        }
        case DS_NAMESPACE::DLayerShellWindow::LayerButtom: {
            //use Normal type will influenced by exclusionZone
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Normal);
            m_window->setFlags(Qt::WindowStaysOnBottomHint);
            break;
        }
        case DS_NAMESPACE::DLayerShellWindow::LayerTop: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Dock);
            break;
        }
        case DS_NAMESPACE::DLayerShellWindow::LayerOverlay: {
            // on deepin Notification will be influenced by exclusionZone,
            // while plasma works all right, maybe deepin kwin bug?
            // FIXME: fix above
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Notification);
            break;
        }
    }
}

void LayerShellEmulation::onPositionChanged()
{
    auto anchors = m_dlayerShellWindow->anchors();
    auto x = m_window->x(), y = m_window->y();
    auto screen = m_window->screen();
    if (anchors & DS_NAMESPACE::DLayerShellWindow::AnchorRight) {
        x = (screen->geometry().right() - m_window->width() - m_dlayerShellWindow->rightMargin());
    }

    if (anchors & DS_NAMESPACE::DLayerShellWindow::AnchorBottom) {
        y = (screen->geometry().bottom() - m_window->height() - m_dlayerShellWindow->bottomMargin());
    }
    if (anchors & DS_NAMESPACE::DLayerShellWindow::AnchorLeft) {
        x = (screen->geometry().left() + m_dlayerShellWindow->leftMargin());
    }
    if (anchors & DS_NAMESPACE::DLayerShellWindow::AnchorTop) {
        y = (screen->geometry().top() + m_dlayerShellWindow->topMargin());
    }

    if (anchors == DS_NAMESPACE::DLayerShellWindow::AnchorNone) {
        x = (screen->geometry().right() - m_window->width()) / 2;
        y = (screen->geometry().height() - m_window->height()) / 2;
    }
    m_window->setX(x), m_window->setY(y);
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#idm45649101327728
  */
void LayerShellEmulation::onExclusionZoneChanged()
{
    auto scaleFactor = qGuiApp->devicePixelRatio();
    auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    xcb_ewmh_connection_t ewmh_connection;
    xcb_intern_atom_cookie_t * cookie = xcb_ewmh_init_atoms(x11Application->connection(), &ewmh_connection);
    xcb_ewmh_init_atoms_replies(&ewmh_connection, cookie, NULL);
    xcb_ewmh_wm_strut_partial_t strut_partial;
    memset(&strut_partial, 0, sizeof(xcb_ewmh_wm_strut_partial_t));
    auto anchors = m_dlayerShellWindow->anchors();
    if ((anchors == DS_NAMESPACE::DLayerShellWindow::AnchorLeft) || (anchors ^ DS_NAMESPACE::DLayerShellWindow::AnchorLeft) == (DS_NAMESPACE::DLayerShellWindow::AnchorTop | DS_NAMESPACE::DLayerShellWindow::AnchorBottom)) {
        strut_partial.left = m_dlayerShellWindow->exclusionZone() * scaleFactor;
        strut_partial.left_start_y = m_window->y();
        strut_partial.left_end_y = m_window->y() + m_window->height();
    } else if ((anchors == DS_NAMESPACE::DLayerShellWindow::AnchorRight) || (anchors ^ DS_NAMESPACE::DLayerShellWindow::AnchorRight) == (DS_NAMESPACE::DLayerShellWindow::AnchorTop | DS_NAMESPACE::DLayerShellWindow::AnchorBottom)) {
        strut_partial.right = m_dlayerShellWindow->exclusionZone() * scaleFactor;
        strut_partial.right_start_y = m_window->y();
        strut_partial.right_end_y = m_window->y() + m_window->height();
    } else if ((anchors == DS_NAMESPACE::DLayerShellWindow::AnchorTop) || (anchors ^ DS_NAMESPACE::DLayerShellWindow::AnchorTop) == (DS_NAMESPACE::DLayerShellWindow::AnchorLeft | DS_NAMESPACE::DLayerShellWindow::AnchorRight)) {
        strut_partial.top = m_dlayerShellWindow->exclusionZone() * scaleFactor;
        strut_partial.top_start_x = m_window->x();
        strut_partial.top_end_x = m_window->x() + m_window->width();
    } else if ((anchors == DS_NAMESPACE::DLayerShellWindow::AnchorBottom) || (anchors ^ DS_NAMESPACE::DLayerShellWindow::AnchorBottom) == (DS_NAMESPACE::DLayerShellWindow::AnchorLeft | DS_NAMESPACE::DLayerShellWindow::AnchorRight)) {
        strut_partial.bottom = m_dlayerShellWindow->exclusionZone() * scaleFactor;
        strut_partial.bottom_start_x = m_window->x();
        strut_partial.bottom_end_x = m_window->x() + m_window->width();
    }

    xcb_ewmh_set_wm_strut_partial(&ewmh_connection, m_window->winId(), strut_partial);
}

// void X11Emulation::onKeyboardInteractivityChanged()
// {
//     // kwin no implentation on wayland
// }
