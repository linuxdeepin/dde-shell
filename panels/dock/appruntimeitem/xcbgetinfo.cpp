// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbgetinfo.h"
#include <QDebug>
#include <QPointer>
#include <QCoreApplication>

#define X11 X11Utils::instance()

namespace dock {
static QPointer<XcbGetInfo> xcbgetinfo;

bool XcbEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (eventType != "xcb_generic_event_t")
        return false;

    auto xcb_event = reinterpret_cast<xcb_generic_event_t*>(message);
    switch (xcb_event->response_type) {
    case XCB_PROPERTY_NOTIFY: {
        auto pE = reinterpret_cast<xcb_property_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->windowPropertyChanged(pE->window, pE->atom);
        break;
    }
    case XCB_CREATE_NOTIFY: {
        auto e = reinterpret_cast<xcb_create_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->eventFilterWindowCreated(e);
        break;
    }
    case XCB_DESTROY_NOTIFY: {
        auto e = reinterpret_cast<xcb_destroy_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->eventFilterWindowDestroyed(e);
    }
    case XCB_ENTER_NOTIFY: {
        auto eN = reinterpret_cast<xcb_enter_notify_event_t *>(xcb_event);
        Q_EMIT xcbgetinfo->windowEnterChanged(eN->event);
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto lN = reinterpret_cast<xcb_leave_notify_event_t *>(xcb_event);
        Q_EMIT xcbgetinfo->windowLeaveChanged(lN->event);
        break;
    }
    }
    return false;
}

XcbGetInfo::XcbGetInfo()
{
    xcbgetinfo = this;
    connect(this, &XcbGetInfo::windowEnterChanged, this, &XcbGetInfo::handleEnterEvent);
    connect(this, &XcbGetInfo::windowLeaveChanged, this, &XcbGetInfo::handleLeaveEvent);
    connect(this, &XcbGetInfo::windowPropertyChanged, this, &XcbGetInfo::handlePropertyNotifyEvent);
    connect(this, &XcbGetInfo::windowPropertyChanged, this, &XcbGetInfo::handlePropertyNotifyEvent);

    connect(this, &XcbGetInfo::eventFilterWindowCreated, this, &XcbGetInfo::handleCreateNotifyEvent);
    connect(this, &XcbGetInfo::eventFilterWindowDestroyed, this, &XcbGetInfo::handleDestroyNotifyEvent);
    connect(this, &XcbGetInfo::windowEnterChangedActive, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowLeaveChangedInactiveName(window, name);
    });
    connect(this, &XcbGetInfo::windowEnterChangedActive, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowEnterChangedActiveName(window, name);
    });

    connect(this, &XcbGetInfo::windowCreatedForeground, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowInfoChangedForeground(name, window);
    });
    connect(this, &XcbGetInfo::windowCreated, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowInfoChanged(name, window);
    });
    connect(this, &XcbGetInfo::windowDestroyed, this, [this](xcb_window_t window) {
        Q_EMIT windowDestroyChanged(window);
    });
}

int XcbGetInfo::startGetinfo()
{
    uint32_t value_list[] = {
        0                               | XCB_EVENT_MASK_PROPERTY_CHANGE        |
        XCB_EVENT_MASK_VISIBILITY_CHANGE    | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY    |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY     | XCB_EVENT_MASK_FOCUS_CHANGE
    };
    xcb_change_window_attributes(X11->getXcbConnection(), X11->getRootWindow(), XCB_CW_EVENT_MASK, value_list);
    xcb_flush(X11->getXcbConnection());
    m_xcbEventGet.reset(new XcbEventFilter());
    qApp->installNativeEventFilter(m_xcbEventGet.get());
    return 0;
}

void XcbGetInfo::handleEnterEvent(xcb_window_t window)
{
    QString m_name = X11->getWindowName(window);
    Q_EMIT windowEnterChangedActive(window,m_name);
}

void XcbGetInfo::handleLeaveEvent(xcb_window_t window)
{
    QString m_name = X11->getWindowName(window);
    Q_EMIT windowLeaveChangedInactive(window,m_name);

}

void XcbGetInfo::handleCreateNotifyEvent(xcb_create_notify_event_t *e)
{
    if (!e->override_redirect) {
        addWindows(e->window);
        QString name = m_windows.value(e->window)->title();
        if (!name.isEmpty()) {
            Q_EMIT windowCreated(e->window, name);
        }
    }
}

void XcbGetInfo::handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e)
{
    Q_EMIT windowDestroyed(e->window);
}

void XcbGetInfo::handlePropertyNotifyEvent(xcb_window_t window, xcb_atom_t atom)
{

    auto x11Window = m_windows.value(window);
    if (!x11Window) {
        return;
    }

    if (atom == X11->getAtomByName("_NET_WM_STATE")) {
        x11Window->updateWindowState();
    }
}

void XcbGetInfo::getAllOpenedWiondws()
{
    QString name;
    const xcb_setup_t *setup = xcb_get_setup(X11->getXcbConnection());
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;
    xcb_window_t m_rootWindow = screen->root;
    m_allOpenedWindows = X11->getWindowClientList(m_rootWindow);
    for (xcb_window_t openedWindows : m_allOpenedWindows) {
        xcb_get_geometry_cookie_t cookie = xcb_get_geometry(X11->getXcbConnection(), openedWindows);
        xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(X11->getXcbConnection(), cookie, nullptr);
        if (!m_windows.contains(openedWindows)) {
            addWindows(openedWindows);
            if(geometry->width>10||geometry->height>10){
                name = m_windows.value(openedWindows)->title();
                Q_EMIT windowCreatedForeground(openedWindows,name);
            }
            else{
                name = m_windows.value(openedWindows)->title();
                Q_EMIT windowCreatedBackground(openedWindows,name);
            }
        }
    }

}

void XcbGetInfo::addWindows(xcb_window_t window)
{
    auto window_get = m_windows.value(window, nullptr);
    if (window_get) return;
    window_get = QSharedPointer<X11Window>{new X11Window(window, this)};
    m_windows.insert(window, window_get);
    connect(m_windows[window].get(), &AbstractWindow::stateChanged, this, [window, this](){
        if(m_windows[window]->m_windowStates.contains(X11->getAtomByName("_NET_WM_STATE_FOCUSED"))) {
            Q_EMIT windowEnterChangedActive(window,X11->getWindowName(window));
        }
        else {
            Q_EMIT windowLeaveChangedInactive(window,X11->getWindowName(window));
        }
    });
    uint32_t value_list[] = { XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_VISIBILITY_CHANGE};
    xcb_change_window_attributes(X11->getXcbConnection(), window, XCB_CW_EVENT_MASK, value_list);
}

}
