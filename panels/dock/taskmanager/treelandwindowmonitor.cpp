// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandwindowmonitor.h"
#include "abstractwindow.h"
#include "abstractwindowmonitor.h"
#include "appitem.h"
#include "taskmanager.h"
#include "treelandwindow.h"

#include <QWindow>

#include <QIODevice>
#include <QPointer>
#include <QVarLengthArray>

#include <cstdint>
#include <utility>

namespace dock {
ForeignToplevelManager::ForeignToplevelManager(TreeLandWindowMonitor* monitor)
    : QWaylandClientExtensionTemplate<ForeignToplevelManager, &QtWayland::treeland_foreign_toplevel_manager_v3::destroy>(1)
    , m_monitor(monitor)
{
}

void ForeignToplevelManager::treeland_foreign_toplevel_manager_v3_toplevel(struct ::treeland_foreign_toplevel_handle_v3 *toplevel)
{
    ForeignToplevelHandle* handle = new ForeignToplevelHandle(toplevel);
    connect(handle, &ForeignToplevelHandle::handlerIsReady, m_monitor, &TreeLandWindowMonitor::handleForeignToplevelHandleAdded, Qt::UniqueConnection);
}

TreeLandWindowMonitor::TreeLandWindowMonitor(QObject *panel, QObject *parent)
    :AbstractWindowMonitor(parent)
    , m_panel(panel)
    , m_fullscreenState(false)
{
}

TreeLandWindowMonitor::~TreeLandWindowMonitor()
{
    releaseDockPreviewContext();
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
    releaseDockPreviewContext();
    m_foreignToplevelManager.reset(nullptr);
}

void TreeLandWindowMonitor::clear()
{
    clearTrackedWindows();
    m_windows.clear();
    releaseDockPreviewContext();
    updateFullscreenState();
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
    if (m_dockPreview)
        QMetaObject::invokeMethod(m_dockPreview, "hideWindowsPreview", Qt::DirectConnection);
}

void TreeLandWindowMonitor::ensureDockPreviewContext(QWindow *anchor)
{
    QObject *context = nullptr;
    if (m_panel)
        QMetaObject::invokeMethod(m_panel, "dockPreviewContext", Qt::DirectConnection,
                                  Q_RETURN_ARG(QObject *, context), Q_ARG(QObject *, anchor));
    if (m_dockPreview == context)
        return;

    releaseDockPreviewContext();
    if (!context)
        return;
    m_dockPreview = context;
    connect(context, SIGNAL(closed()), this, SIGNAL(previewShouldClear()));
    connect(context, &QObject::destroyed, this, &TreeLandWindowMonitor::previewShouldClear);
}

void TreeLandWindowMonitor::releaseDockPreviewContext()
{
    hideItemPreview();
    if (m_dockPreview)
        disconnect(m_dockPreview, nullptr, this, nullptr);
    m_dockPreview.clear();
}

void TreeLandWindowMonitor::requestPreview(QAbstractItemModel *sourceModel,
                                           QWindow *relativePositionItem,
                                           int32_t previewXoffset,
                                           int32_t previewYoffset,
                                           uint32_t direction)
{
    if (!sourceModel || !m_foreignToplevelManager || !m_foreignToplevelManager->isActive()) {
        return;
    }

    QVarLengthArray<uint32_t> array;
    for (int i = 0; i < sourceModel->rowCount(); ++i) {
        QModelIndex index = sourceModel->index(i, 0);
        uint32_t winId = index.data(TaskManager::WinIdRole).toUInt();
        if (winId != 0) {
            array.append(winId);
        }
    }

    if (array.isEmpty())
        return;

    ensureDockPreviewContext(relativePositionItem);
    if (!m_dockPreview)
        return;

    const QByteArray windows(reinterpret_cast<const char *>(array.constData()), array.size() * sizeof(uint32_t));
    QMetaObject::invokeMethod(m_dockPreview, "showWindowsPreview", Qt::DirectConnection,
                              Q_ARG(QByteArray, windows), Q_ARG(int, previewXoffset),
                              Q_ARG(int, previewYoffset), Q_ARG(uint, direction));
}

void TreeLandWindowMonitor::requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
    uint32_t winId = index.data(TaskManager::WinIdRole).toUInt();
    auto window = m_windows.value(winId, nullptr);
    if (window) {
        window->setWindowIconGeometry(qobject_cast<QWindow *>(delegate), geometry);
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

    connect(window.data(), &AbstractWindow::stateChanged, this, &TreeLandWindowMonitor::updateFullscreenState, Qt::UniqueConnection);

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
        destroyWindow(window.get());
        m_windows.remove(id);
        updateFullscreenState();
    }
}

void TreeLandWindowMonitor::updateFullscreenState()
{
    for (auto window : std::as_const(m_windows)) {
        if (window && window->isFullscreen() && !window->isMinimized()) {
            if (!m_fullscreenState) {
                m_fullscreenState = true;
                emit windowFullscreenChanged(true);
            }
            return;
        }
    }

    if (m_fullscreenState) {
        m_fullscreenState = false;
        emit windowFullscreenChanged(false);
    }
}
}
