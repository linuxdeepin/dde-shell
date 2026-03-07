// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupwindow.h"

#include <QtQuick/QQuickItem>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

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

QFont PopupWindow::font() const
{
    return m_font;
}

void PopupWindow::setFont(const QFont &font)
{
    m_hasExplicitFont = true;
    QFont resolved = font.resolve(QGuiApplication::font());
    if (m_font.resolveMask() == resolved.resolveMask() && m_font == resolved)
        return;
    m_font = resolved;
    propagateFontToContentItem(m_font);
    emit fontChanged();
}

void PopupWindow::resetFont()
{
    m_hasExplicitFont = false;
    QFont defaultFont;
    if (m_font == defaultFont)
        return;
    m_font = defaultFont;
    propagateFontToContentItem(m_font);
    emit fontChanged();
}

void PopupWindow::propagateFontToContentItem(const QFont &font)
{
    if (QQuickItem *ci = contentItem())
        QQuickControlPrivate::updateFontRecur(ci, font);
}

DS_END_NAMESPACE
