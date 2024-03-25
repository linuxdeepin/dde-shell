// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "dsglobal.h"
#include "waylandwindow.h"
#include "abstractwindow.h"
#include "waylandwindowmonitor.h"
#include "abstractwindowmonitor.h"

#include <QPointer>
#include <QIODevice>
#include <QVarLengthArray>

#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <algorithm>
#include <cstdint>
#include <iterator>

DS_BEGIN_NAMESPACE

namespace dock {
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

TreelandDockPreviewContext::TreelandDockPreviewContext(struct ::treeland_dock_preview_context_v1 *context)
    : QWaylandClientExtensionTemplate<TreelandDockPreviewContext>(1)
    , m_hideTimer(new QTimer(this))
    , m_isPreviewEntered(false)
    , m_isDockMouseAreaEnter(false)
{
    init(context);

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(500);

    connect(m_hideTimer, &QTimer::timeout, this, [this](){
        if (!m_isDockMouseAreaEnter && !m_isPreviewEntered) {
            close();
        }
    }, Qt::QueuedConnection);
}

TreelandDockPreviewContext::~TreelandDockPreviewContext()
{
    destroy();
}

void TreelandDockPreviewContext::showWindowsPreview(QByteArray windowsId, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    m_isDockMouseAreaEnter = true;
    show(windowsId, previewXoffset, previewYoffset, direction);
}

void TreelandDockPreviewContext::hideWindowsPreview()
{
    m_isDockMouseAreaEnter = false;
    m_hideTimer->start();
}

void TreelandDockPreviewContext::treeland_dock_preview_context_v1_enter()
{
    m_isPreviewEntered = true;
}

void TreelandDockPreviewContext::treeland_dock_preview_context_v1_leave()
{
    m_isPreviewEntered = false;
    m_hideTimer->start();
}

WaylandWindowMonitor::WaylandWindowMonitor(QObject* parent)
    :AbstractWindowMonitor(parent)
{
}

void WaylandWindowMonitor::start()
{
    m_foreignToplevelManager.reset(new ForeignToplevelManager(this));
    connect(m_foreignToplevelManager.get(), &ForeignToplevelManager::newForeignToplevelHandle, this, &WaylandWindowMonitor::handleForeignToplevelHandleAdded);
}
void WaylandWindowMonitor::stop()
{
    m_foreignToplevelManager.reset(nullptr);
}

QPointer<AbstractWindow> WaylandWindowMonitor::getWindowByWindowId(ulong windowId)
{
    return m_windows.value(windowId).get();
}

void WaylandWindowMonitor::presentWindows(QList<uint32_t> windows)
{

}

void WaylandWindowMonitor::showItemPreview(const QPointer<AppItem> &item, QObject* relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    if (m_dockPreview.isNull()) {
        auto window = qobject_cast<QWindow*>(relativePositionItem);
        if (!window) return;
        auto waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
        if (!waylandWindow) return;

        auto context = m_foreignToplevelManager->get_dock_preview_context(waylandWindow->wlSurface());
        m_dockPreview.reset(new TreelandDockPreviewContext(context));
    }

    QVarLengthArray array = QVarLengthArray<uint32_t>();

    std::transform(item->getAppendWindows().begin(), item->getAppendWindows().end(), std::back_inserter(array), [](const QPointer<AbstractWindow>& window){
        return window->id();
    });
    
    QByteArray windowIds(reinterpret_cast<char*>(array.data()));
    m_dockPreview->showWindowsPreview(windowIds, previewXoffset, previewYoffset, direction);
}

void WaylandWindowMonitor::hideItemPreview()
{
    if (m_dockPreview.isNull()) return;
    m_dockPreview->hideWindowsPreview();
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
}

DS_END_NAMESPACE
