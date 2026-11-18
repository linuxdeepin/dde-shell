// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstractwindowmonitor.h"
#include "qwayland-treeland-foreign-toplevel-manager-unstable-v3.h"
#include "treelandwindow.h"

#include <QHash>
#include <QList>
#include <QObject>

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

class TreeLandWindowMonitor : public AbstractWindowMonitor
{
    Q_OBJECT

public:
    explicit TreeLandWindowMonitor(QObject *panel, QObject *parent = nullptr);
    ~TreeLandWindowMonitor() override;
    virtual void start() override;
    virtual void stop() override;
    virtual void clear() override;

    virtual QPointer<AbstractWindow> getWindowByWindowId(ulong windowId) override;

    virtual void presentWindows(QList<uint32_t> windows) override;
    virtual void hideItemPreview() override;

    void
    requestPreview(QAbstractItemModel *sourceModel, QWindow *relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction) override;
    void requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const override;

private Q_SLOTS:
    friend class ForeignToplevelManager;

    void handleForeignToplevelHandleAdded();
    void handleForeignToplevelHandleRemoved();
    void updateFullscreenState();

private:
    void ensureDockPreviewContext(QWindow *anchor);
    void releaseDockPreviewContext();

    QPointer<QObject> m_panel;
    QPointer<QObject> m_dockPreview;
    QHash<ulong, QSharedPointer<TreeLandWindow>> m_windows;
    QScopedPointer<ForeignToplevelManager> m_foreignToplevelManager;
    bool m_fullscreenState;
};
}
