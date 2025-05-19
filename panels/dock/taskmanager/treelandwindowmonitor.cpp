// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandwindowmonitor.h"
#include "abstractwindow.h"
#include "abstractwindowmonitor.h"
#include "appitem.h"
#include "taskmanager.h"
#include "treelandwindow.h"

#include <QPointer>
#include <QIODevice>
#include <QVarLengthArray>

#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <cstdint>

namespace dock {
ForeignToplevelManager::ForeignToplevelManager(TreeLandWindowMonitor* monitor)
    : QWaylandClientExtensionTemplate<ForeignToplevelManager>(1)
    , m_monitor(monitor)
{
}

void ForeignToplevelManager::treeland_foreign_toplevel_manager_v1_toplevel(struct ::treeland_foreign_toplevel_handle_v1 *toplevel)
{
    ForeignToplevelHandle* handle = new ForeignToplevelHandle(toplevel);
    connect(handle, &ForeignToplevelHandle::handlerIsReady, m_monitor, &TreeLandWindowMonitor::handleForeignToplevelHandleAdded, Qt::UniqueConnection);
}

TreeLandDockPreviewContext::TreeLandDockPreviewContext(struct ::treeland_dock_preview_context_v1 *context)
    : QWaylandClientExtensionTemplate<TreeLandDockPreviewContext>(1)
    , m_isPreviewEntered(false)
    , m_isDockMouseAreaEnter(false)
    , m_hideTimer(new QTimer(this))
{
    init(context);

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(800);

    connect(m_hideTimer, &QTimer::timeout, this, [this](){
        if (!m_isDockMouseAreaEnter && !m_isPreviewEntered) {
            emit closed();
            close();
        }
    }, Qt::QueuedConnection);
}

TreeLandDockPreviewContext::~TreeLandDockPreviewContext()
{
    destroy();
}

void TreeLandDockPreviewContext::showWindowsPreview(QByteArray windowsId, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    m_isDockMouseAreaEnter = true;
    show(windowsId, previewXoffset, previewYoffset, direction);
}

void TreeLandDockPreviewContext::hideWindowsPreview()
{
    m_isDockMouseAreaEnter = false;
    m_hideTimer->start();
}

void TreeLandDockPreviewContext::treeland_dock_preview_context_v1_enter()
{
    m_isPreviewEntered = true;
}

void TreeLandDockPreviewContext::treeland_dock_preview_context_v1_leave()
{
    m_isPreviewEntered = false;
    m_hideTimer->start();
}

TreeLandWindowMonitor::TreeLandWindowMonitor(QObject* parent)
    :AbstractWindowMonitor(parent)
    , m_fullscreenState(false)
{
}

void TreeLandWindowMonitor::start()
{
    m_foreignToplevelManager.reset(new ForeignToplevelManager(this));
    connect(m_foreignToplevelManager.get(), &ForeignToplevelManager::newForeignToplevelHandle, this, &TreeLandWindowMonitor::handleForeignToplevelHandleAdded);
    connect(m_foreignToplevelManager.get(), &ForeignToplevelManager::activeChanged, this, [this]{
        if (!m_foreignToplevelManager->isActive()) {
            clear();
        }
    });
}

void TreeLandWindowMonitor::stop()
{
    m_foreignToplevelManager.reset(nullptr);
}


void TreeLandWindowMonitor::clear()
{
    m_windows.clear();
    m_dockPreview.reset(nullptr);
}

QPointer<AbstractWindow> TreeLandWindowMonitor::getWindowByWindowId(ulong windowId)
{
    return m_windows.value(windowId).get();
}

void TreeLandWindowMonitor::presentWindows(QList<uint32_t> windows)
{
    Q_UNUSED(windows)
}

void TreeLandWindowMonitor::hideItemPreview()
{
    if (m_dockPreview.isNull())
        return;
    m_dockPreview->hideWindowsPreview();
}

void TreeLandWindowMonitor::requestPreview(QAbstractItemModel *sourceModel,
                                           QWindow *relativePositionItem,
                                           int32_t previewXoffset,
                                           int32_t previewYoffset,
                                           uint32_t direction)
{
    if (!m_foreignToplevelManager->isActive()) {
        return;
    }

    if (m_dockPreview.isNull()) {
        auto window = relativePositionItem;
        if (!window) return;
        auto waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
        if (!waylandWindow) return;

        auto context = m_foreignToplevelManager->get_dock_preview_context(waylandWindow->wlSurface());
        m_dockPreview.reset(new TreeLandDockPreviewContext(context));
        connect(window, &QWindow::visibleChanged, m_dockPreview.get(), [this]() {
            m_dockPreview.reset();
        });
        connect(m_dockPreview.get(), &TreeLandDockPreviewContext::closed, this, [this]() {
            // 当预览关闭时，发出信号清空过滤状态
            emit previewShouldClear();
        });
    }

    m_dockPreview->m_isDockMouseAreaEnter = true;

    if (sourceModel && sourceModel->rowCount() > 0) {
        QVarLengthArray<uint32_t> array;
        for (int i = 0; i < sourceModel->rowCount(); ++i) {
            QModelIndex index = sourceModel->index(i, 0);
            uint32_t winId = index.data(TaskManager::WinIdRole).toUInt();
            if (winId != 0) {
                array.append(winId);
            }
        }

        if (!array.isEmpty()) {
            QByteArray byteArray;
            int size = array.size() * sizeof(uint32_t);
            byteArray.resize(size);
            memcpy(byteArray.data(), array.constData(), size);
            m_dockPreview->showWindowsPreview(byteArray, previewXoffset, previewYoffset, direction);
        } else {
            m_dockPreview->show_tooltip("???", previewXoffset, previewYoffset, direction);
        }
    } else {
        m_dockPreview->show_tooltip("???", previewXoffset, previewYoffset, direction);
    }
}

void TreeLandWindowMonitor::handleForeignToplevelHandleAdded()
{
    auto handle = qobject_cast<ForeignToplevelHandle*>(sender());
    if (!handle) {
        return;
    }

    auto id = handle->id();
    auto window = m_windows.value(id, nullptr);
    connect(handle, &ForeignToplevelHandle::handlerIsDeleted,this, &TreeLandWindowMonitor::handleForeignToplevelHandleRemoved, Qt::UniqueConnection);

    if (!window) {
        window = QSharedPointer<TreeLandWindow>(new TreeLandWindow(id));
        m_windows.insert(id, window);
    }

    connect(window.data(), &AbstractWindow::stateChanged, this, [=] {
        for (auto w: m_windows) {
            if (w->isFullscreen() && !m_fullscreenState) {
                m_fullscreenState = true;
                emit windowFullscreenChanged(true);
                return;
            }
        }
        if (m_fullscreenState) {
            m_fullscreenState = false;
            emit windowFullscreenChanged(false);
        }
    });

    window->setForeignToplevelHandle(handle);

    if (window->isReady()) {
        trackWindow(window.get());
        Q_EMIT AbstractWindowMonitor::windowAdded(static_cast<QPointer<AbstractWindow>>(window.get()));
    }
}

void TreeLandWindowMonitor::handleForeignToplevelHandleRemoved()
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

