// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "dockpanel.h"
#include "x11dockhelper.h"


#include <qobjectdefs.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QPointer>

DS_BEGIN_NAMESPACE
namespace dock {
const uint16_t monitorSize = 15;
class XcbEventFilter: public QAbstractNativeEventFilter
{
public:
    XcbEventFilter(X11DockHelper* helper)
        : m_helper(helper)
    {}

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
    {
        if (eventType != "xcb_generic_event_t" || m_helper.isNull())
            return false;

        auto xcb_event = reinterpret_cast<xcb_generic_event_t*>(message);
        switch (xcb_event->response_type) {
            case XCB_ENTER_NOTIFY: {
                auto eN = reinterpret_cast<xcb_enter_notify_event_t*>(xcb_event);
                m_helper->m_isHoverIn = true;
                Q_EMIT m_helper->mouseInDockAreaChanged();
                break;
            }
            case XCB_LEAVE_NOTIFY: {
                auto lN = reinterpret_cast<xcb_leave_notify_event_t*>(xcb_event);
                m_helper->m_isHoverIn = false;
                Q_EMIT m_helper->mouseInDockAreaChanged();
                break;
            }
        }
        return false;
    };

private:
    QPointer<X11DockHelper> m_helper;
};
X11DockHelper::X11DockHelper(DockPanel* panel)
    : DockHelper(panel)
{
    xcb_screen_t *screen;
    auto connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    m_triggerWindow = xcb_generate_id(connection);
    uint32_t values_list[] = {1};
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, m_triggerWindow, screen->root, 0, 0, 1000, 1000, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_CW_OVERRIDE_REDIRECT, values_list);

    if (parent()->hideMode() != KeepShowing) {
        uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW};
        xcb_change_window_attributes(connection, m_triggerWindow, XCB_CW_EVENT_MASK, values);
        xcb_map_window(connection, m_triggerWindow);
    }

    xcb_flush(connection);

    connect(panel, &DockPanel::geometryChanged, this, [this](){
        updateDockTriggerArea();
    });

    connect(panel, &DockPanel::hideModeChanged, this, [this, connection](){
        if (parent()->hideMode() == KeepShowing) {
            xcb_unmap_window(connection, m_triggerWindow);
        } else {
            uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW};
            xcb_change_window_attributes(connection, m_triggerWindow, XCB_CW_EVENT_MASK, values);
            xcb_map_window(connection, m_triggerWindow);
        }
    });

    connect(this, &X11DockHelper::mouseInDockAreaChanged, this, [this, connection](){
        if (mouseInDockArea()) {
            QTimer::singleShot(1000, [this, connection](){
                if (mouseInDockArea()) {
                    xcb_unmap_window(connection, m_triggerWindow);
                }
            });
        } else {
            xcb_map_window(connection, m_triggerWindow);
        }
    });

    qGuiApp->installNativeEventFilter(new XcbEventFilter(this));
}

void X11DockHelper::updateDockTriggerArea()
{
    auto connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    auto rect = parent()->frontendWindowRect();
    int values[4];

    switch (parent()->position()) {
        case Top: {
            values[0] = int(rect.left());
            values[1] = int(rect.top());
            values[2] = int(rect.width());
            values[3] = int(monitorSize);
            break;
        }
        case Bottom: {
            values[0] = int(rect.left());
            values[1] = int((rect.top() + rect.height() - monitorSize + 1));
            values[2] = int(rect.width());
            values[3] = int(monitorSize);
            break;
        }
        case Left: {
            values[0] = int(rect.left());
            values[1] = int(rect.top());
            values[2] = int(monitorSize);
            values[3] = int(rect.height());
            break;
        }
        case Right: {
            values[0] = int((rect.left() + rect.width() - monitorSize + 1));
            values[1] = int(rect.top());
            values[2] = int(monitorSize);
            values[3] = int(rect.height());
            break;
        }
    }

    xcb_configure_window(connection, m_triggerWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(connection);
}

bool X11DockHelper::mouseInDockArea()
{
    return m_isHoverIn;
}
}
DS_END_NAMESPACE
