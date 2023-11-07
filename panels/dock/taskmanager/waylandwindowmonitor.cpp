// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "waylandwindow.h"
#include "abstractwindow.h"
#include "waylandwindowmonitor.h"
#include "abstractwindowmonitor.h"

#include <QPointer>

DS_BEGIN_NAMESPACE

namespace dock {
ExtForeignToplevelList::ExtForeignToplevelList(WaylandWindowMonitor* monitor)
    : QWaylandClientExtensionTemplate<ExtForeignToplevelList>(1)
    , QtWayland::ext_foreign_toplevel_list_v1()
    , m_monitor(monitor)
{
}

void ExtForeignToplevelList::ext_foreign_toplevel_list_v1_toplevel(struct ::ext_foreign_toplevel_handle_v1 *toplevel)
{
    ExtForeignToplevelHandle *handle = new ExtForeignToplevelHandle(toplevel);
    connect(handle, &ExtForeignToplevelHandle::handlerIsReady, m_monitor,&WaylandWindowMonitor::handleExtForeignToplevelHandleAdded, Qt::UniqueConnection);
}

void ExtForeignToplevelList::ext_foreign_toplevel_list_v1_finished()
{

}

ForeignToplevelManager::ForeignToplevelManager(WaylandWindowMonitor* monitor)
    : QWaylandClientExtensionTemplate<ForeignToplevelManager>(1)
    , m_monitor(monitor)
{
}

void ForeignToplevelManager::ztreeland_foreign_toplevel_manager_v1_toplevel(struct ::ztreeland_foreign_toplevel_handle_v1 *toplevel)
{
    ForeignToplevelHandle* handle = new ForeignToplevelHandle(toplevel);
    connect(handle, &ForeignToplevelHandle::handlerIsReady, m_monitor,&WaylandWindowMonitor::handleForeignToplevelHandleAdded, Qt::UniqueConnection);
}

WaylandWindowMonitor::WaylandWindowMonitor(QObject* parent)
    :AbstractWindowMonitor(parent)
{
}

void WaylandWindowMonitor::start()
{
    m_extForeignToplevelList.reset(new ExtForeignToplevelList(this));
    connect(m_extForeignToplevelList.get(), &ExtForeignToplevelList::newExtForeignToplevelHandle, this, &WaylandWindowMonitor::handleExtForeignToplevelHandleAdded);

    m_foreignToplevelManager.reset(new ForeignToplevelManager(this));
    connect(m_foreignToplevelManager.get(), &ForeignToplevelManager::newForeignToplevelHandle, this, &WaylandWindowMonitor::handleForeignToplevelHandleAdded);
}
void WaylandWindowMonitor::stop()
{
    m_extForeignToplevelList.reset(nullptr);
    m_foreignToplevelManager.reset(nullptr);
}

QPointer<AbstractWindow> WaylandWindowMonitor::getWindowByWindowId(ulong windowId)
{
    return m_windows.value(windowId).get();
}

void WaylandWindowMonitor::presentWindows(QStringList windows)
{

}

void WaylandWindowMonitor::handleForeignToplevelHandleAdded()
{
    auto handle = qobject_cast<ForeignToplevelHandle*>(sender());
    if (!handle) {
        return;
    }

    auto id = handle->id();
    auto window = m_windows.value(id, nullptr);
    connect(handle, &ForeignToplevelHandle::handlerIsDeleted,this, &WaylandWindowMonitor::handleForeignToplevelHandleRemoved, Qt::UniqueConnection);

    if (!window) {
        window = QSharedPointer<WaylandWindow>(new WaylandWindow(id));
        m_windows.insert(id, window);
    }

    window->setForeignToplevelHandle(handle);

    if (window->isReady())
        Q_EMIT AbstractWindowMonitor::windowAdded(static_cast<QPointer<AbstractWindow>>(window.get()));
}

void WaylandWindowMonitor::handleExtForeignToplevelHandleAdded()
{
    auto handle = qobject_cast<ExtForeignToplevelHandle*>(sender());
    if (!handle) {
        return;
    }

    auto id = handle->id();
    auto window = m_windows.value(id, nullptr);
    connect(handle, &ExtForeignToplevelHandle::handlerIsDeleted,this, &WaylandWindowMonitor::handleExtForeignToplevelHandleRemoved, Qt::UniqueConnection);

    if (!window) {
        window = QSharedPointer<WaylandWindow>(new WaylandWindow(id, this));
        m_windows.insert(id, window);
    }

    window->setExtForeignToplevelHandle(handle);
    if (window->isReady())
        Q_EMIT AbstractWindowMonitor::windowAdded(static_cast<QPointer<AbstractWindow>>(window.get()));
}

void WaylandWindowMonitor::handleForeignToplevelHandleRemoved()
{
    auto handle = qobject_cast<ForeignToplevelHandle*>(sender());
    if (!handle) {
        return;
    }

    auto id = handle->id();
    auto window = m_windows.value(id, nullptr);

    if (window) {
        m_windows.remove(id);
    }
}
void WaylandWindowMonitor::handleExtForeignToplevelHandleRemoved()
{
    auto handle = qobject_cast<ExtForeignToplevelHandle*>(sender());
    if (!handle) {
        return;
    }

    auto id = handle->id();
    auto window = m_windows.value(id, nullptr);

    if (window) {
        m_windows.remove(id);
    }
}
}

DS_END_NAMESPACE
