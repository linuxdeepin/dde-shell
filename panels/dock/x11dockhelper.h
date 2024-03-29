// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "dsglobal.h"
#include <xcb/xproto.h>

DS_BEGIN_NAMESPACE

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
    friend class XcbEventFilter;

private:
    bool m_isHoverIn;
    QString m_registerKey;
    xcb_window_t m_triggerWindow;
};
}

DS_END_NAMESPACE
