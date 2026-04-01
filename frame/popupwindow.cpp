// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupwindow.h"

#include <xcb/xcb.h>

DS_BEGIN_NAMESPACE
PopupWindow::PopupWindow(QWindow *parent)
    : QQuickApplicationWindow(parent)
{
    // minimum size is 1x1 to prevent protocols error on wayland
    setMinimumSize(QSize(1, 1));

    // maximum size is the screen size, to prevent the window from being too large and causing
    // the video memory to explode
    auto setMaximumSize = [this]() {
        auto screen = QWindow::screen();
        if (screen) {
            this->setMaximumSize(screen->size());
        }
    };

    connect(this, &QWindow::screenChanged, this, setMaximumSize);
    setMaximumSize();

    connect(this, &QWindow::visibleChanged, this, [this]() {
        if (!isVisible()) {
            setX11GrabFocusTransition(false);
        }
    });
}

void PopupWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickApplicationWindow::mouseReleaseEvent(event);
    auto rect = geometry();
    if (!m_dragging && !rect.contains(event->globalPosition().toPoint()) && type() == Qt::Popup) {
        QMetaObject::invokeMethod(this, &QWindow::close, Qt::QueuedConnection);
    }
    m_dragging = false;
    m_pressing = false;
}

void PopupWindow::mousePressEvent(QMouseEvent *event)
{
    m_pressing = true;
    return QQuickApplicationWindow::mousePressEvent(event);
}

void PopupWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressing) {
        m_dragging = true;
    }
    return QQuickApplicationWindow::mouseMoveEvent(event);
}

void PopupWindow::setX11GrabFocusTransition(bool transition)
{
    if (m_x11GrabFocusTransition == transition) {
        return;
    }

    m_x11GrabFocusTransition = transition;
    Q_EMIT x11GrabFocusTransitionChanged();
}

bool PopupWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType == QByteArrayLiteral("xcb_generic_event_t")) {
        auto *event = static_cast<xcb_generic_event_t *>(message);
        if (!event) {
            return QQuickApplicationWindow::nativeEvent(eventType, message, result);
        }

        const uint8_t responseType = event->response_type & ~0x80;
        if (responseType == XCB_FOCUS_IN || responseType == XCB_FOCUS_OUT) {
            auto *focusEvent = reinterpret_cast<xcb_focus_in_event_t *>(event);
            if (focusEvent->event == static_cast<xcb_window_t>(winId())) {
                if (responseType == XCB_FOCUS_OUT && focusEvent->mode == XCB_NOTIFY_MODE_GRAB) {
                    setX11GrabFocusTransition(true);
                    Q_EMIT x11FocusOutByGrab();
                } else if (responseType == XCB_FOCUS_IN && focusEvent->mode == XCB_NOTIFY_MODE_UNGRAB) {
                    Q_EMIT x11FocusInByUngrab();
                    setX11GrabFocusTransition(false);
                } else if (responseType == XCB_FOCUS_IN && focusEvent->mode != XCB_NOTIFY_MODE_GRAB) {
                    setX11GrabFocusTransition(false);
                }
            }
        }
    }

    return QQuickApplicationWindow::nativeEvent(eventType, message, result);
}

DS_END_NAMESPACE
