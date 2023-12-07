// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "waylandwindow.h"
#include "abstractwindow.h"

#include <sys/types.h>

#include <QLoggingCategory>

#include <private/qwaylandnativeinterface_p.h>

Q_LOGGING_CATEGORY(waylandwindowLog, "dde.shell.dock.taskmanager.waylandwindow")

DS_BEGIN_NAMESPACE
namespace dock {
ForeignToplevelHandle::ForeignToplevelHandle(struct ::ztreeland_foreign_toplevel_handle_v1 *object)
    : QWaylandClientExtensionTemplate<ForeignToplevelHandle>(1)
    , QtWayland::ztreeland_foreign_toplevel_handle_v1(object)
    , m_pid(0)
    , m_isReady(false)
{}

ulong ForeignToplevelHandle::id() const
{
    return m_identifier;
}

pid_t ForeignToplevelHandle::pid() const
{
    return m_pid;
}

QString ForeignToplevelHandle::title() const
{
    return m_title;
}

QList<uint32_t> ForeignToplevelHandle::state() const
{
    return m_states;
}

bool ForeignToplevelHandle::isReady() const
{
    return m_isReady;
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_pid(uint32_t pid)
{
    if (pid == m_pid) return;

    m_pid = pid;
    Q_EMIT pidChanged();
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_title(const QString &title)
{
    if (title == m_title) return;

    m_title = title;
    Q_EMIT titleChanged();
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_app_id(const QString &app_id)
{
    if (app_id == m_appId) return;
    m_appId = app_id;
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_identifier(const QString &identifier)
{
    m_identifier = identifier.toULong();
}

void ForeignToplevelHandle::ztreeland_foreign_toplevel_handle_v1_state(wl_array *state)
{
    m_states.clear();
    const uint32_t* items = reinterpret_cast<const uint32_t*>(state->data);
    int itemCount = state->size / sizeof(uint32_t);

    for (int i = 0; i < itemCount; i++) {
        m_states.append(items[i]);
    }

    Q_EMIT isActiveChanged();
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
    return m_foreignToplevelHandle ? m_foreignToplevelHandle->title() : "";
}

void WaylandWindow::updateIsActive()
{
}

bool WaylandWindow::isActive()
{
    return m_foreignToplevelHandle->state().contains(Active);
}

bool WaylandWindow::shouldSkip()
{
    return false;
}

bool WaylandWindow::isMinimized()
{
    return m_foreignToplevelHandle->state().contains(Minimized);
}

bool WaylandWindow::allowClose()
{
    return true;
}

void WaylandWindow::close()
{
    m_foreignToplevelHandle->close();
}

void WaylandWindow::activate()
{
    QtWaylandClient::QWaylandNativeInterface *app =
            static_cast<QtWaylandClient::QWaylandNativeInterface *>(
                    QGuiApplication::platformNativeInterface());
    auto seat = app->seat();
    m_foreignToplevelHandle->activate(seat);
}

void WaylandWindow::maxmize()
{
    m_foreignToplevelHandle->set_maximized();
}

void WaylandWindow::minimize()
{
    m_foreignToplevelHandle->set_minimized();
}

void WaylandWindow::killClient()
{
    m_foreignToplevelHandle->close();
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

bool WaylandWindow::isReady()
{
    return m_foreignToplevelHandle->isReady();
}

void WaylandWindow::setForeignToplevelHandle(ForeignToplevelHandle* handle)
{
    if (m_foreignToplevelHandle && m_foreignToplevelHandle.get() == handle) return;
    m_foreignToplevelHandle.reset(handle);
    m_id = m_foreignToplevelHandle->id();

    connect(m_foreignToplevelHandle.get(), &ForeignToplevelHandle::pidChanged, this, &AbstractWindow::pidChanged);
    connect(m_foreignToplevelHandle.get(), &ForeignToplevelHandle::titleChanged, this, &AbstractWindow::titleChanged);
    connect(m_foreignToplevelHandle.get(), &ForeignToplevelHandle::isActiveChanged, this, &AbstractWindow::isActiveChanged);
}

}
DS_END_NAMESPACE
