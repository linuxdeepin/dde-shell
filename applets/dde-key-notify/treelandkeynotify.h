// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "qwayland-treeland-keyboard-state-notify-unstable-v1.h"

#include <QPointer>
#include <QtWaylandClient/QWaylandClientExtension>

struct treeland_keyboard_state_watcher_v1;

DS_BEGIN_NAMESPACE
namespace keynotify
{

class TreelandKeyWatcher;

class TreelandKeyNotify : public QWaylandClientExtensionTemplate<TreelandKeyNotify>, public QtWayland::treeland_keyboard_state_notify_manager_v1
{
    Q_OBJECT

public:
    explicit TreelandKeyNotify(QObject *parent = nullptr);

    void setCapsLockEnabled(bool enabled);

Q_SIGNALS:
    void capsLockChanged(bool locked);
    void numLockChanged(bool locked);

private Q_SLOTS:
    void updateWatcher();

private:
    TreelandKeyWatcher *createWatcher(QObject *parent = nullptr);

private:
    bool m_capsLockEnabled = false;
    QPointer<TreelandKeyWatcher> m_watcher;
};

class TreelandKeyWatcher : public QObject, public QtWayland::treeland_keyboard_state_watcher_v1
{
    Q_OBJECT

public:
    explicit TreelandKeyWatcher(struct ::treeland_keyboard_state_watcher_v1 *object, QObject *parent = nullptr);

    void watchLocks(bool watchCapsLock);

Q_SIGNALS:
    void stateChanged(uint32_t modifier, uint32_t state);

protected:
    void treeland_keyboard_state_watcher_v1_state_changed(uint32_t modifier, uint32_t state) override;
};

}
DS_END_NAMESPACE
