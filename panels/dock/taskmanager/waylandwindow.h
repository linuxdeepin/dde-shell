// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "abstractwindow.h"
#include "qwayland-ext-foreign-toplevel-list-v1.h"
#include "qwayland-ztreeland-foreign-toplevel-manager-v1.h"

#include <sys/types.h>

#include <QScopedPointer>
#include <QtWaylandClient/QWaylandClientExtension>

DS_BEGIN_NAMESPACE
namespace dock {
class ExtForeignToplevelHandle : public QWaylandClientExtensionTemplate<ExtForeignToplevelHandle>, public QtWayland::ext_foreign_toplevel_handle_v1
{
    Q_OBJECT
public:
    explicit ExtForeignToplevelHandle(struct ::ext_foreign_toplevel_handle_v1 *object);
    
    ulong id() const;
    bool isReady() const;
    QString title() const;

Q_SIGNALS:
    // those signals only used for windowmonitor
    void handlerIsReady();
    void handlerIsDeleted();

protected:
    void ext_foreign_toplevel_handle_v1_closed() override;
    void ext_foreign_toplevel_handle_v1_done() override;
    void ext_foreign_toplevel_handle_v1_title(const QString &title) override;
    void ext_foreign_toplevel_handle_v1_app_id(const QString &app_id) override;
    void ext_foreign_toplevel_handle_v1_identifier(const QString &identifier) override;

private:
    bool m_isReady;
    QString m_title;
    QString m_appId;
    ulong m_identifier;
};

class ForeignToplevelHandle : public QWaylandClientExtensionTemplate<ForeignToplevelHandle>, public QtWayland::ztreeland_foreign_toplevel_handle_v1
{
    Q_OBJECT

public:
    explicit ForeignToplevelHandle(struct ::ztreeland_foreign_toplevel_handle_v1 *object);
    bool isReady() const;
    ulong id() const;
    pid_t pid() const;

Q_SIGNALS:
    // those signals only used for windowmonitor
    void handlerIsReady();
    void handlerIsDeleted();

protected:
    void ztreeland_foreign_toplevel_handle_v1_app_id(const QString &app_id) override;
    void ztreeland_foreign_toplevel_handle_v1_pid(uint32_t pid) override;
    void ztreeland_foreign_toplevel_handle_v1_done() override;
    void ztreeland_foreign_toplevel_handle_v1_closed() override;
    void ztreeland_foreign_toplevel_handle_v1_identifier(const QString &identifier) override;

private:
    uint32_t m_pid;
    bool m_isReady;
    QString m_appId;
    ulong m_identifier;
};

class WaylandWindowMonitor;

class WaylandWindow : public AbstractWindow
{
    Q_OBJECT

public:
    ~WaylandWindow();

    virtual uint32_t id() override;
    virtual pid_t pid() override;
    virtual QString icon() override;
    virtual QString title() override;
    virtual bool isActive() override;
    virtual bool shouldSkip() override;
    virtual bool isMinimized() override;
    virtual bool allowClose() override;

    virtual void close() override;
    virtual void activate() override;
    virtual void maxmize() override;
    virtual void minimize() override;
    virtual void killClient() override;

private:
    virtual void updatePid() override;
    virtual void updateIcon() override;
    virtual void updateTitle() override;
    virtual void updateIsActive() override;
    virtual void updateShouldSkip() override;
    virtual void updateAllowClose() override;
    virtual void updateIsMinimized() override;

    void setForeignToplevelHandle(ForeignToplevelHandle* handle);
    void setExtForeignToplevelHandle(ExtForeignToplevelHandle* handle);

    bool isReady();

private:
    friend class WaylandWindowMonitor;

    WaylandWindow(uint32_t id, QObject *parent = nullptr);

private:
    uint32_t m_id;

    QScopedPointer<ExtForeignToplevelHandle> m_extForeignToplevelHandle;
    QScopedPointer<ForeignToplevelHandle> m_foreignToplevelHandle;
};
}
DS_END_NAMESPACE
