// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "dsglobal.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>


namespace dock {
class X11DockHelper;

class XcbEventFilter: public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    XcbEventFilter(X11DockHelper* helper);
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override;

private:
    bool inTriggerArea(xcb_window_t win) const;
    void processEnterLeave(xcb_window_t win, bool enter);

    QPointer<X11DockHelper> m_helper;
    QTimer *m_timer;
};

class DockTriggerArea : public QObject
{
    Q_OBJECT

public:
    DockTriggerArea(DockPanel *panel, X11DockHelper *helper, QScreen *qScreen);
    ~DockTriggerArea();

    xcb_window_t triggerWindow() const { return m_triggerWindow; }
    QScreen *screen() const { return m_screen; }
    void enableWakeArea();
    void disableWakeArea();
    void mouseEnter();
    void mouseLeave();
public Q_SLOTS:
    void updateDockTriggerArea();
    void onTriggerTimer();
    void onHoldingTimer();

private:
    const QRect matchDockTriggerArea();

    DockPanel *m_panel;
    X11DockHelper *m_helper;
    QScreen *m_screen;
    xcb_window_t m_triggerWindow;
    xcb_window_t m_rootWindow;
    xcb_connection_t *m_connection;
    bool m_enableWakeArea;

    QTimer* m_triggerTimer;
    QTimer* m_holdingTimer;
};


class X11DockHelper : public DockHelper
{
    Q_OBJECT

public:
    X11DockHelper(DockPanel* panel);
    bool mouseInDockArea() override;
    QList<DockTriggerArea*> triggerAreas() const { return m_areas; }
    
    void mouseEnterDockArea();
    void mouseLeaveDockArea();

public Q_SLOTS:
    void updateDockTriggerArea() override;

private:
    inline void createdWakeArea();

private:
    friend class XcbEventFilter;

private:
    bool m_isHoverIn;
    QString m_registerKey;
    QList<DockTriggerArea*> m_areas;
};
}

