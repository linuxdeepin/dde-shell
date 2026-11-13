// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstractwindowmonitor.h"
#include "qwayland-treeland-foreign-toplevel-manager-unstable-v3.h"
#include "treelandtooltipsurface.h"
#include "treelandwindow.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

#include <QtWaylandClient/QWaylandClientExtension>

namespace dock {
class ForeignToplevelManager : public QWaylandClientExtensionTemplate<ForeignToplevelManager, &QtWayland::treeland_foreign_toplevel_manager_v3::destroy>, public QtWayland::treeland_foreign_toplevel_manager_v3
{
    Q_OBJECT
public:
    explicit ForeignToplevelManager(TreeLandWindowMonitor* monitor);

Q_SIGNALS:
    void newForeignToplevelHandle(ForeignToplevelHandle *handle);

protected:
    void treeland_foreign_toplevel_manager_v3_toplevel(struct ::treeland_foreign_toplevel_handle_v3 *toplevel) override;

private:
    TreeLandWindowMonitor* m_monitor;
};

class TreeLandDockPreviewContext : public QWaylandClientExtensionTemplate<TreeLandDockPreviewContext>, public QtWayland::treeland_dock_preview_context_v3
{
    Q_OBJECT

public:
    explicit TreeLandDockPreviewContext(struct ::treeland_dock_preview_context_v3 *);
    ~TreeLandDockPreviewContext();
    void showWindowsPreview(QByteArray windowsId, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);
    void showTooltip(struct ::wl_surface *surface, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction);
    void hideWindowsPreview();
    // tooltip 没有可以停留的弹窗区域，收起时用一个比预览短得多的宽限期。
    void hideTooltip();

Q_SIGNALS:
    void entered();
    void exited();
    void closed();

protected:
    virtual void treeland_dock_preview_context_v3_enter() override;
    virtual void treeland_dock_preview_context_v3_leave() override;
    friend class TreeLandWindowMonitor;

private:
    bool m_isPreviewEntered;
    bool m_isDockMouseAreaEnter;
    QTimer* m_hideTimer;
};

class TreeLandWindowMonitor : public AbstractWindowMonitor
{
    Q_OBJECT

public:
    explicit TreeLandWindowMonitor(QObject* parent = nullptr);
    ~TreeLandWindowMonitor() override;
    virtual void start() override;
    virtual void stop() override;
    virtual void clear() override;

    virtual QPointer<AbstractWindow> getWindowByWindowId(ulong windowId) override;

    virtual void presentWindows(QList<uint32_t> windows) override;
    virtual void hideItemPreview() override;

    void
    requestPreview(QAbstractItemModel *sourceModel, QWindow *relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction) override;
    bool requestTooltip(QObject *tooltipOwner,
                        const QString &text,
                        QWindow *relativePositionItem,
                        int32_t previewXoffset,
                        int32_t previewYoffset,
                        uint32_t direction) override;
    void hideTooltip(QObject *tooltipOwner) override;
    void requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const override;

private Q_SLOTS:
    friend class ForeignToplevelManager;

    void handleForeignToplevelHandleAdded();
    void handleForeignToplevelHandleRemoved();
    void updateFullscreenState();

private:
    void ensureDockPreviewContext(QWindow *relativePositionItem);
    // 每个 tooltip 一个 surface：按宿主 QML 项懒创建，之后一直复用（协议规定
    // surface 的 tooltip role 一旦赋予就不再释放，复用它不会重复建立 role）。
    TreelandTooltipSurface *tooltipSurface(QObject *tooltipOwner, QWindow *relativePositionItem);
    void handleTooltipOwnerDestroyed(QObject *tooltipOwner);
    void destroyTooltipSurfaces();

    QHash<ulong, QSharedPointer<TreeLandWindow>> m_windows;
    QScopedPointer<ForeignToplevelManager> m_foreignToplevelManager;
    QScopedPointer<TreeLandDockPreviewContext> m_dockPreview;
    QHash<QObject *, TreelandTooltipSurface *> m_tooltipSurfaces;
    // 当前由 context 显示的那个 tooltip，用来实现 hideTooltip() 的归属判断。
    TreelandTooltipSurface *m_shownTooltip = nullptr;

    bool m_fullscreenState;
};
}
