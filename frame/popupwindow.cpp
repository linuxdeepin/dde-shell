// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupwindow.h"

DS_BEGIN_NAMESPACE
PopupWindow::PopupWindow(QWindow *parent)
    : QQuickWindowQmlImpl(parent)
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
}

void PopupWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickWindowQmlImpl::mouseReleaseEvent(event);
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
    return QQuickWindowQmlImpl::mousePressEvent(event);
}

void PopupWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressing) {
        m_dragging = true;
    }
    return QQuickWindowQmlImpl::mouseMoveEvent(event);
}

DS_END_NAMESPACE
