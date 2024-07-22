// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/utility_x11_p.h"

#include <QLoggingCategory>
#include <QMouseEvent>

#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

static X11Utility *instance()
{
    return dynamic_cast<X11Utility*>(Utility::instance());
}

X11Utility::X11Utility()
{
    m_display = XOpenDisplay("");
    if (!m_display) {
        qCWarning(dsLog) << "Failed to open XDisplay.";
    }
}

_XDisplay *X11Utility::getDisplay()
{
    return m_display;
}

void X11Utility::deliverMouseEvent(uint8_t qMouseButton, int x, int y)
{
    uint8_t mouseButton = XCB_BUTTON_INDEX_1;
    switch (qMouseButton) {
    case Qt::MiddleButton:
        mouseButton = XCB_BUTTON_INDEX_2;
        break;
    case Qt::RightButton:
        mouseButton = XCB_BUTTON_INDEX_3;
        break;
    }

    auto dis = getDisplay();
    if (!dis)
        return;
    XTestFakeMotionEvent(dis, 0, x, y, 0);
    XFlush(dis);
    XTestFakeButtonEvent(dis, mouseButton, true, 0);
    XFlush(dis);
    XTestFakeButtonEvent(dis, mouseButton, false, 0);
    XFlush(dis);
}

bool X11Utility::grabKeyboard(QWindow *target, bool grab)
{
    if (target) {
        qCDebug(dsLog) << "grab keyboard for the window:" << target->winId();
        return target->setKeyboardGrabEnabled(grab);
    }
    return false;
}

bool X11Utility::grabMouse(QWindow *target, bool grab)
{
    if (target) {
        qCDebug(dsLog) << "grab mouse for the window:" << target->winId();
        auto filter = new MouseGrabEventFilter(target);
        qApp->installEventFilter(filter);
        QObject::connect(filter, &MouseGrabEventFilter::outsideMousePressed, target, [filter, target] () {
            qCDebug(dsLog) << "ungrab mouse for the window:" << target->winId();
            target->close();
            qApp->removeEventFilter(filter);
            target->setMouseGrabEnabled(false);
            filter->deleteLater();
        });
        return target->setMouseGrabEnabled(grab);
    }
    return false;
}

MouseGrabEventFilter::MouseGrabEventFilter(QWindow *target)
    : QObject (target)
    , m_target(target)
{

}

bool MouseGrabEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_target)
        return false;
    if (event->type() == QEvent::MouseButtonRelease) {
        mousePressEvent(static_cast<QMouseEvent *>(event));
    }
    return false;
}

void MouseGrabEventFilter::mousePressEvent(QMouseEvent *e)
{
    const auto bounding = m_target->geometry();
    const auto pos = e->globalPosition();
    if ((e->position().toPoint().isNull() && !pos.isNull()) ||
        !bounding.contains(pos.toPoint())) {
        emit outsideMousePressed();
        instance()->deliverMouseEvent(e->button(), pos.x(), pos.y());
        return;
    }
}

DS_END_NAMESPACE
