// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "waylandwindow.h"
#include "abstractwindowmonitor.h"
#include "qwayland-treeland-foreign-toplevel-manager-v1.h"

#include <QHash>
#include <QList>
#include <QObject>

#include <QtWaylandClient/QWaylandClientExtension>

DS_BEGIN_NAMESPACE
namespace dock {
class ForeignToplevelManager : public QWaylandClientExtensionTemplate<ForeignToplevelManager>, public QtWayland::ztreeland_foreign_toplevel_manager_v1
{
    Q_OBJECT
public:
    explicit ForeignToplevelManager(WaylandWindowMonitor* monitor);

Q_SIGNALS:
    void newForeignToplevelHandle(ForeignToplevelHandle *handle);

protected:
    void ztreeland_foreign_toplevel_manager_v1_toplevel(struct ::ztreeland_foreign_toplevel_handle_v1 *toplevel) override;

private:
    WaylandWindowMonitor* m_monitor;
};

class WaylandWindowMonitor : public AbstractWindowMonitor
{
    Q_OBJECT

public:
    explicit WaylandWindowMonitor(QObject* parent = nullptr);
    virtual void start() override;
    virtual void stop() override;

    virtual QPointer<AbstractWindow> getWindowByWindowId(ulong windowId) override;

    virtual void presentWindows(QStringList windows) override;

private Q_SLOTS:
    friend class ForeignToplevelManager;
    friend class ExtForeignToplevelList;

    void handleForeignToplevelHandleAdded();
    void handleExtForeignToplevelHandleAdded();

    void handleForeignToplevelHandleRemoved();
    void handleExtForeignToplevelHandleRemoved();

private:
    QHash<ulong, QSharedPointer<WaylandWindow>> m_windows;
    QScopedPointer<ForeignToplevelManager> m_foreignToplevelManager;

};
}
DS_END_NAMESPACE
