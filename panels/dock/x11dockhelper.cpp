// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "constants.h"
#include "dsglobal.h"
#include "dockpanel.h"
#include "x11dockhelper.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QPointer>

namespace dock {
const uint16_t monitorSize = 15;

XcbEventFilter::XcbEventFilter(X11DockHelper* helper)
    : m_helper(helper)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(200);

    connect(m_timer, &QTimer::timeout, this, [this](){
        m_helper->mouseLeaveDockArea();
    });
}

bool XcbEventFilter::inTriggerArea(xcb_window_t win) const
{ 
    for (auto area: m_helper->m_areas) {
        if (area->triggerWindow() == win)
            return true;
    }
    return false;
}

void XcbEventFilter::processEnterLeave(xcb_window_t win, bool enter)
{
    for (auto area: m_helper->m_areas) {
        if (area->triggerWindow() == win) {
            if (enter) {
                area->mouseEnter();
            } else {
                area->mouseLeave();
            }
            return;
        }
    }

// dock enter/leave
    if (enter) {
        m_helper->mouseEnterDockArea();
    } else {
        m_helper->mouseLeaveDockArea();
    }
}

bool XcbEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (eventType != "xcb_generic_event_t" || m_helper.isNull())
        return false;

    auto xcb_event = reinterpret_cast<xcb_generic_event_t*>(message);
    switch (xcb_event->response_type) {
    case XCB_ENTER_NOTIFY: {
        auto eN = reinterpret_cast<xcb_enter_notify_event_t*>(xcb_event);
        if (m_timer->isActive()) {
            if (!inTriggerArea(eN->event)) {
                m_timer->stop();
                break;
            }
        }
        processEnterLeave(eN->event, true);
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto lN = reinterpret_cast<xcb_leave_notify_event_t*>(xcb_event);
        if (!inTriggerArea(lN->event)) {
            m_timer->start();
            break;
        }
        processEnterLeave(lN->event, false);
        break;
    }
    }
    return false;
};

X11DockHelper::X11DockHelper(DockPanel* panel)
    : DockHelper(panel)
{
    connect(parent(), &DockPanel::rootObjectChanged, this, &X11DockHelper::createdWakeArea);
    connect(panel, &DockPanel::hideStateChanged, this, &X11DockHelper::updateDockTriggerArea);
    connect(panel, &DockPanel::showInPrimaryChanged, this, &X11DockHelper::updateDockTriggerArea);
    qGuiApp->installNativeEventFilter(new XcbEventFilter(this));
}

void X11DockHelper::updateDockTriggerArea()
{
    for (auto area: m_areas) {
        area->updateDockTriggerArea();
    }
}

bool X11DockHelper::mouseInDockArea()
{
    return m_isHoverIn;
}

void X11DockHelper::mouseEnterDockArea()
{
    m_isHoverIn = true;
    Q_EMIT mouseInDockAreaChanged();
}

void X11DockHelper::mouseLeaveDockArea()
{

    m_isHoverIn = false;
    Q_EMIT mouseInDockAreaChanged();

}

void X11DockHelper::createdWakeArea()
{
    for (auto screen: qApp->screens()) {
        DockTriggerArea* area = new DockTriggerArea(parent(), this, screen);
        m_areas.append(area);
    }
}

DockTriggerArea::DockTriggerArea(DockPanel *panel, X11DockHelper *helper, QScreen *qScreen)
    : QObject(panel)
    , m_panel(panel)
    , m_helper(helper)
    , m_screen(qScreen)
    , m_enableWakeArea(false)
    , m_triggerTimer(new QTimer(this))
    , m_holdingTimer(new QTimer(this))
{
    m_triggerTimer->setSingleShot(true);
    m_triggerTimer->setInterval(1000);
    m_holdingTimer->setSingleShot(true);
    m_holdingTimer->setInterval(200);

    connect(m_triggerTimer, &QTimer::timeout, this, &DockTriggerArea::onTriggerTimer);
    connect(m_holdingTimer, &QTimer::timeout, this, &DockTriggerArea::onHoldingTimer);
    connect(panel, &DockPanel::positionChanged, this, &DockTriggerArea::updateDockTriggerArea);
    m_connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;
    m_rootWindow = screen->root;
    m_triggerWindow = xcb_generate_id(m_connection);
    xcb_flush(m_connection);
    updateDockTriggerArea();
}

DockTriggerArea::~DockTriggerArea()
{
    disableWakeArea();
}

void DockTriggerArea::enableWakeArea()
{
    if (m_enableWakeArea)
        return;
    uint32_t values_list[] = {1};
    QRect rect = matchDockTriggerArea();
    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT, m_triggerWindow, m_rootWindow, rect.x(), rect.y(), rect.width(), rect.height(), 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_CW_OVERRIDE_REDIRECT, values_list);
    uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW};
    xcb_change_window_attributes(m_connection, m_triggerWindow, XCB_CW_EVENT_MASK, values);
    xcb_map_window(m_connection, m_triggerWindow);
    m_enableWakeArea = true;
}

void DockTriggerArea::disableWakeArea()
{
    if (!m_enableWakeArea || m_holdingTimer->isActive())
        return;
    xcb_destroy_window(m_connection, m_triggerWindow);
    m_enableWakeArea = false;
}

void DockTriggerArea::mouseEnter()
{
    m_triggerTimer->start();
}

void DockTriggerArea::mouseLeave()
{
    m_triggerTimer->stop();
}

const QRect DockTriggerArea::matchDockTriggerArea()
{
    QRect triggerArea;
    auto rect = m_screen->geometry();
    int values[4];

    switch (m_panel->position()) {
    case Top: {
        triggerArea.setX(rect.left());
        triggerArea.setY(rect.top());
        triggerArea.setWidth(rect.width());
        triggerArea.setHeight(monitorSize);
        break;
    }
    case Bottom: {
        triggerArea.setX(rect.left());
        triggerArea.setY(rect.top() + rect.height() - monitorSize + 1);
        triggerArea.setWidth(rect.width());
        triggerArea.setHeight(monitorSize);
        break;
    }
    case Left: {
        triggerArea.setX(rect.left());
        triggerArea.setY(rect.top());
        triggerArea.setWidth(monitorSize);
        triggerArea.setHeight(rect.height());
        break;
    }
    case Right: {
        triggerArea.setX(rect.left() + rect.width() - monitorSize + 1);
        triggerArea.setY(rect.top());
        triggerArea.setWidth(monitorSize);
        triggerArea.setHeight(rect.height());
        break;
    }
    }

    return triggerArea;
}

void DockTriggerArea::updateDockTriggerArea()
{
    auto isDockAllowedShownOnThisScreen = [this]() -> bool {
        return (m_panel->showInPrimary() && m_screen == qApp->primaryScreen()) || !m_panel->showInPrimary();
    };

    auto isDockShownThisScreen = [this]() -> bool {
        return m_panel->dockScreen() == m_screen && m_panel->hideState() == Show;
    };

    if (isDockAllowedShownOnThisScreen() && !isDockShownThisScreen()) {
        enableWakeArea();
    } else {
        disableWakeArea();
        return;
    }

    QRect rect = matchDockTriggerArea();
    auto connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    int values[4] = {rect.x(), rect.y(), rect.width(), rect.height()};

    xcb_configure_window(connection, m_triggerWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(connection);
}
void DockTriggerArea::onTriggerTimer()
{
    m_holdingTimer->start();
    if (m_panel->dockScreen() != m_screen) {
        m_panel->setDockScreen(m_screen);
        m_helper->updateDockTriggerArea();
    }
    m_panel->setHideState(Show);
}
void DockTriggerArea::onHoldingTimer()
{
    if (m_panel->dockScreen() == m_screen && m_panel->hideState() == Show) {
      disableWakeArea();
    } else {
      enableWakeArea();
    }
}
} // namespace dock
