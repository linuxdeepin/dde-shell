// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "abstractwindow.h"

#include <mutex>
#include <sys/types.h>
#include <xcb/xproto.h>

#include <QObject>

DS_BEGIN_NAMESPACE
namespace dock {
class X11Window : public AbstractWindow
{
    Q_OBJECT
public:
    ~X11Window();
    virtual uint32_t id() override;
    virtual pid_t pid() override;
    virtual QString icon() override;
    virtual QString title() override;
    virtual bool isActive() override;
    virtual bool shouldSkip() override;
    virtual bool isMinimized() override;
    virtual bool allowClose() override;
    virtual bool isAttention() override;

    virtual void close() override;
    virtual void activate() override;
    virtual void maxmize() override;
    virtual void minimize() override;
    virtual void killClient() override;

private:
    friend class X11WindowMonitor;
    X11Window(xcb_window_t winid, QObject *parent = nullptr);

private:
    virtual void updatePid() override;
    virtual void updateIcon() override;
    virtual void updateTitle() override;
    virtual void updateIsActive() override;
    virtual void updateShouldSkip() override;
    virtual void updateAllowClose() override;
    virtual void updateIsMinimized() override;

    void updateWindowState();
    inline void checkWindowState();

    void updateWindowAllowedActions();
    inline void checkWindowAllowedActions();

    void updateWindowTypes();
    inline void checkWindowTypes();

    bool hasWmStateModal();
    bool hasWmStateSkipTaskBar();
    bool isActionMinimizeAllowed();

private:
    xcb_window_t m_windowID;
    pid_t m_pid;
    QString m_icon;
    QString m_title;

    QList<xcb_atom_t> m_windowStates;
    QList<xcb_atom_t> m_windowTypes;
    QList<xcb_atom_t> m_windowAllowedActions;

    std::once_flag m_windowTypeFlag;
    std::once_flag m_windowStateFlag;
    std::once_flag m_windowAllowedActionsFlag;
};
}
DS_END_NAMESPACE