// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandwindowmonitor.h"
#include "abstractwindow.h"
#include "abstractwindowmonitor.h"
#include "appitem.h"
#include "taskmanager.h"
#include "treelandwindow.h"

#include <QDebug>
#include <QIODevice>
#include <QPointer>
#include <QVarLengthArray>

#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <cstdint>
#include <utility>

namespace dock {
namespace
{
// 悬停收起的宽限期，预览与 tooltip 共用：既要覆盖指针在两个相邻 dock 项之间
// 移动，也要覆盖从 dock 图标移进预览弹窗的那一小段时间。
constexpr int HideGrace = 350;
}

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

TreeLandDockPreviewContext::TreeLandDockPreviewContext(struct ::treeland_dock_preview_context_v3 *context)
    : QWaylandClientExtensionTemplate<TreeLandDockPreviewContext>(1)
    , m_isPreviewEntered(false)
    , m_isDockMouseAreaEnter(false)
    , m_hideTimer(new QTimer(this))
{
    init(context);

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(HideGrace);

    connect(m_hideTimer, &QTimer::timeout, this, [this](){
        if (!m_isDockMouseAreaEnter && !m_isPreviewEntered) {
            emit closed();
            close();
        }
    }, Qt::QueuedConnection);
}

TreeLandDockPreviewContext::~TreeLandDockPreviewContext()
{
    if (isInitialized())
        destroy();
}

void TreeLandDockPreviewContext::showWindowsPreview(QByteArray windowsId, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    m_isDockMouseAreaEnter = true;
    show(windowsId, previewXoffset, previewYoffset, direction);
}

void TreeLandDockPreviewContext::showTooltip(struct ::wl_surface *surface, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    m_isDockMouseAreaEnter = true;
    // TreelandTooltipSurface::setText committed the content right before this
    // call, so the compositor has something to show.
    show_tooltip_surface(surface, previewXoffset, previewYoffset, direction);
}

void TreeLandDockPreviewContext::hideWindowsPreview()
{
    m_isDockMouseAreaEnter = false;
    m_hideTimer->start(HideGrace);
}

void TreeLandDockPreviewContext::hideTooltip()
{
    // 与预览共用同一个定时器和同一个宽限期。
    m_isDockMouseAreaEnter = false;
    m_hideTimer->start(HideGrace);
}

void TreeLandDockPreviewContext::treeland_dock_preview_context_v3_enter()
{
    m_isPreviewEntered = true;
}

void TreeLandDockPreviewContext::treeland_dock_preview_context_v3_leave()
{
    m_isPreviewEntered = false;
    m_hideTimer->start(HideGrace);
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
    destroyTooltipSurfaces();
}

TreeLandWindowMonitor::~TreeLandWindowMonitor()
{
    destroyTooltipSurfaces();
}

void TreeLandWindowMonitor::clear()
{
    clearTrackedWindows();
    m_windows.clear();
    m_dockPreview.reset(nullptr);
    destroyTooltipSurfaces();
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
    if (m_dockPreview.isNull())
        return;
    m_dockPreview->hideWindowsPreview();
}

void TreeLandWindowMonitor::ensureDockPreviewContext(QWindow *relativePositionItem)
{
    if (!m_dockPreview.isNull())
        return;

    auto window = relativePositionItem;
    if (!window)
        return;
    auto waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle());
    if (!waylandWindow)
        return;

    auto context = m_foreignToplevelManager->get_dock_preview_context(waylandWindow->wlSurface());
    m_dockPreview.reset(new TreeLandDockPreviewContext(context));
    connect(window, &QWindow::visibleChanged, m_dockPreview.get(), [this]() {
        m_dockPreview.reset();
        m_shownTooltip = nullptr;
    });
    connect(m_dockPreview.get(), &TreeLandDockPreviewContext::closed, this, [this]() {
        // 当预览关闭时，发出信号清空过滤状态
        m_shownTooltip = nullptr;
        emit previewShouldClear();
    });
}

TreelandTooltipSurface *TreeLandWindowMonitor::tooltipSurface(QObject *tooltipOwner, QWindow *relativePositionItem)
{
    if (!tooltipOwner)
        return nullptr;

    if (auto *surface = m_tooltipSurfaces.value(tooltipOwner))
        return surface;

    auto *waylandWindow = relativePositionItem ? dynamic_cast<QtWaylandClient::QWaylandWindow *>(relativePositionItem->handle()) : nullptr;
    if (!waylandWindow || !waylandWindow->display())
        return nullptr;

    auto *surface = new TreelandTooltipSurface(waylandWindow->display());
    m_tooltipSurfaces.insert(tooltipOwner, surface);
    // 宿主 QML 项销毁时一起释放，避免为已经不存在的 tooltip 留着 surface 和 buffer。
    connect(tooltipOwner, &QObject::destroyed, this, &TreeLandWindowMonitor::handleTooltipOwnerDestroyed);
    return surface;
}

void TreeLandWindowMonitor::handleTooltipOwnerDestroyed(QObject *tooltipOwner)
{
    auto *surface = m_tooltipSurfaces.take(tooltipOwner);
    if (!surface)
        return;

    if (surface == m_shownTooltip) {
        // surface 马上就要销毁，先放开 context 的占用状态，否则 context 会一直
        // 认为自己还被 dock 鼠标区域占着，永远走不到关闭。
        m_shownTooltip = nullptr;
        if (!m_dockPreview.isNull())
            m_dockPreview->hideTooltip();
    }
    delete surface;
}

void TreeLandWindowMonitor::destroyTooltipSurfaces()
{
    m_shownTooltip = nullptr;
    for (auto *surface : std::as_const(m_tooltipSurfaces))
        delete surface;
    m_tooltipSurfaces.clear();
}

void TreeLandWindowMonitor::requestPreview(QAbstractItemModel *sourceModel,
                                           QWindow *relativePositionItem,
                                           int32_t previewXoffset,
                                           int32_t previewYoffset,
                                           uint32_t direction)
{
    if (!m_foreignToplevelManager || !m_foreignToplevelManager->isActive()) {
        return;
    }

    if (!sourceModel || sourceModel->rowCount() == 0) {
        qWarning() << "requestPreview called without any window";
        return;
    }

    ensureDockPreviewContext(relativePositionItem);
    if (m_dockPreview.isNull())
        return;

    QVarLengthArray<uint32_t> array;
    for (int i = 0; i < sourceModel->rowCount(); ++i) {
        QModelIndex index = sourceModel->index(i, 0);
        uint32_t winId = index.data(TaskManager::WinIdRole).toUInt();
        if (winId != 0) {
            array.append(winId);
        }
    }

    if (array.isEmpty()) {
        qWarning() << "requestPreview called without any valid window id";
        return;
    }

    QByteArray byteArray;
    int size = array.size() * sizeof(uint32_t);
    byteArray.resize(size);
    memcpy(byteArray.data(), array.constData(), size);
    // 预览会顶掉当前显示的 tooltip，之后那个 tooltip 再关就不该动这个预览。
    m_shownTooltip = nullptr;
    m_dockPreview->showWindowsPreview(byteArray, previewXoffset, previewYoffset, direction);
}

bool TreeLandWindowMonitor::requestTooltip(QObject *tooltipOwner,
                                           const QString &text,
                                           QWindow *relativePositionItem,
                                           int32_t previewXoffset,
                                           int32_t previewYoffset,
                                           uint32_t direction)
{
    if (!m_foreignToplevelManager || !m_foreignToplevelManager->isActive())
        return false;

    ensureDockPreviewContext(relativePositionItem);
    if (m_dockPreview.isNull())
        return false;

    auto *surface = tooltipSurface(tooltipOwner, relativePositionItem);
    if (!surface)
        return false;

    const qreal devicePixelRatio = relativePositionItem ? relativePositionItem->devicePixelRatio() : 1.0;
    if (surface->setText(text, devicePixelRatio).isEmpty())
        return false;

    m_dockPreview->showTooltip(surface->surface(), previewXoffset, previewYoffset, direction);
    m_shownTooltip = surface;
    return true;
}

void TreeLandWindowMonitor::hideTooltip(QObject *tooltipOwner)
{
    // 每个 tooltip 有独立的 surface，只有要关的正是当前显示的那个时才收起；
    // 否则会把另一个 tooltip 刚显示的内容一起关掉。
    auto *surface = m_tooltipSurfaces.value(tooltipOwner);
    if (!surface || surface != m_shownTooltip)
        return;

    m_shownTooltip = nullptr;
    if (!m_dockPreview.isNull())
        m_dockPreview->hideTooltip();
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
