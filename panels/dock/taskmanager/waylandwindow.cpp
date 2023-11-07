// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "waylandwindow.h"
#include "abstractwindow.h"

#include <sys/types.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(waylandwindowLog, "dde.shell.dock.taskmanager.waylandwindow")

DS_BEGIN_NAMESPACE
namespace dock {
ExtForeignToplevelHandle::ExtForeignToplevelHandle(struct ::ext_foreign_toplevel_handle_v1 *object)
    : QWaylandClientExtensionTemplate<ExtForeignToplevelHandle>(1)
    , QtWayland::ext_foreign_toplevel_handle_v1(object)
    , m_isReady(false)
{}

void ExtForeignToplevelHandle::ext_foreign_toplevel_handle_v1_closed()
{
    Q_EMIT handlerIsDeleted();
}

bool ExtForeignToplevelHandle::isReady() const
{
    return m_isReady;
}

ulong ExtForeignToplevelHandle::id() const
{
    return m_identifier;
}

QString ExtForeignToplevelHandle::title() const
{
    return m_title;
}

void ExtForeignToplevelHandle::ext_foreign_toplevel_handle_v1_title(const QString& title)
{
    m_title = title;
}

void ExtForeignToplevelHandle::ext_foreign_toplevel_handle_v1_app_id(const QString &app_id)
{
    m_appId = app_id;
}

void ExtForeignToplevelHandle::ext_foreign_toplevel_handle_v1_identifier(const QString &identifier)
{
    m_identifier = identifier.toULong();
}

void ExtForeignToplevelHandle::ext_foreign_toplevel_handle_v1_done()
{
    m_isReady = true;
    Q_EMIT handlerIsReady();
}

ForeignToplevelHandle::ForeignToplevelHandle(struct ::ztreeland_foreign_toplevel_handle_v1 *object)
    : QWaylandClientExtensionTemplate<ForeignToplevelHandle>(1)
    , QtWayland::ztreeland_foreign_toplevel_handle_v1(object)
    , m_pid(0)
{}

ulong ForeignToplevelHandle::id() const
{
    return m_identifier;
}

pid_t ForeignToplevelHandle::pid() const
{
    return m_pid;
}

bool ForeignToplevelHandle::isReady() const
{
    return m_isReady;
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_app_id(const QString &app_id)
{
    m_appId = app_id;
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_pid(uint32_t pid)
{
    m_pid = pid;
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_done()
{
    m_isReady = true;
    Q_EMIT handlerIsReady();
}
void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_closed()
{
    Q_EMIT handlerIsDeleted();
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_identifier(const QString &identifier)
{
    m_identifier = identifier.toULong();
}

WaylandWindow::WaylandWindow(uint32_t id, QObject *parent)
    : AbstractWindow(parent)
    , m_id(id)
{
    qCDebug(waylandwindowLog()) << "wayland window created";
}

WaylandWindow::~WaylandWindow()
{
    qCDebug(waylandwindowLog()) << "wayland window destoried";
}

uint32_t WaylandWindow::id()
{
    return m_id;
}

pid_t WaylandWindow::pid()
{
    return m_foreignToplevelHandle ? m_foreignToplevelHandle->pid() : 0;
}

QString WaylandWindow::icon()
{
    return "";
}

QString WaylandWindow::title()
{
    return m_extForeignToplevelHandle ? m_extForeignToplevelHandle->title() : "";
}

void WaylandWindow::updateIsActive()
{
}

bool WaylandWindow::isActive()
{
    return false;
}

bool WaylandWindow::shouldSkip()
{
    return false;
}

bool WaylandWindow::isMinimized()
{
    return false;
}

bool WaylandWindow::allowClose()
{
    return true;
}

void WaylandWindow::close()
{

}

void WaylandWindow::activate()
{

}

void WaylandWindow::maxmize()
{
    
}

void WaylandWindow::minimize()
{

}

void WaylandWindow::killClient()
{

}

void WaylandWindow::updatePid()
{

}
void WaylandWindow::updateIcon()
{

}

void WaylandWindow::updateTitle()
{

}

void WaylandWindow::updateShouldSkip()
{

}

void WaylandWindow::updateAllowClose()
{

}

void WaylandWindow::updateIsMinimized()
{

}

void WaylandWindow::setForeignToplevelHandle(ForeignToplevelHandle* handle)
{
    if (m_foreignToplevelHandle && m_foreignToplevelHandle.get() == handle) return;
    m_foreignToplevelHandle.reset(handle);
    m_id = m_foreignToplevelHandle->id();
}

void WaylandWindow::setExtForeignToplevelHandle(ExtForeignToplevelHandle* handle)
{
    if (m_extForeignToplevelHandle && m_extForeignToplevelHandle.get() == handle) return;
    m_extForeignToplevelHandle.reset(handle);
    m_id = m_extForeignToplevelHandle->id();
}

bool WaylandWindow::isReady()
{
    return m_extForeignToplevelHandle && m_foreignToplevelHandle;
}

}
DS_END_NAMESPACE
