// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "dsglobal.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>


namespace dock {
class X11DockHelper : public DockHelper
{
    Q_OBJECT

public:
    X11DockHelper(DockPanel* panel);
    bool mouseInDockArea() override;

public Q_SLOTS:
    void updateDockTriggerArea() override;

private:
    inline void createdWakeArea();
    inline void destoryWakeArea();

private:
    friend class XcbEventFilter;

private:
    bool m_isHoverIn;
    QString m_registerKey;
    xcb_window_t m_triggerWindow;
    xcb_window_t m_rootWindow;
    xcb_connection_t* m_connection;
    QTimer* m_destoryTimer;
};
}

