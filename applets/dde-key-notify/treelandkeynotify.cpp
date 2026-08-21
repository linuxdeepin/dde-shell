// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandkeynotify.h"

#include "wayland-treeland-keyboard-state-notify-unstable-v1-client-protocol.h"

DS_BEGIN_NAMESPACE
namespace keynotify
{

TreelandKeyNotify::TreelandKeyNotify(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandKeyNotify>(treeland_keyboard_state_notify_manager_v1_interface.version)
{
    setParent(parent);
    connect(this, &TreelandKeyNotify::activeChanged, this, &TreelandKeyNotify::updateWatcher);
}

void TreelandKeyNotify::setCapsLockEnabled(bool enabled)
{
    if (m_capsLockEnabled == enabled) {
        return;
    }

    m_capsLockEnabled = enabled;
    updateWatcher();
}

void TreelandKeyNotify::updateWatcher()
{
    if (!isActive()) {
        if (m_watcher) {
            m_watcher->deleteLater();
            m_watcher = nullptr;
        }
        return;
    }

    if (!m_watcher) {
        m_watcher = createWatcher(this);
        if (!m_watcher) {
            return;
        }

        connect(m_watcher, &TreelandKeyWatcher::stateChanged, this, [this](uint32_t modifier, uint32_t state) {
            if (modifier == TreelandKeyWatcher::modifier_caps_lock) {
                if (state == TreelandKeyWatcher::modifier_state_locked) {
                    Q_EMIT capsLockChanged(true);
                } else if (state == TreelandKeyWatcher::modifier_state_unlocked) {
                    Q_EMIT capsLockChanged(false);
                }
            } else if (modifier == TreelandKeyWatcher::modifier_num_lock) {
                if (state == TreelandKeyWatcher::modifier_state_locked) {
                    Q_EMIT numLockChanged(true);
                } else if (state == TreelandKeyWatcher::modifier_state_unlocked) {
                    Q_EMIT numLockChanged(false);
                }
            }
        });
    }

    m_watcher->watchLocks(m_capsLockEnabled);
}

TreelandKeyWatcher *TreelandKeyNotify::createWatcher(QObject *parent)
{
    if (!isActive()) {
        return nullptr;
    }

    auto *watcher = get_keyboard_state_watcher(nullptr);
    if (!watcher) {
        return nullptr;
    }

    return new TreelandKeyWatcher(watcher, parent);
}

TreelandKeyWatcher::TreelandKeyWatcher(struct ::treeland_keyboard_state_watcher_v1 *object, QObject *parent)
    : QObject(parent)
    , QtWayland::treeland_keyboard_state_watcher_v1(object)
{
}

void TreelandKeyWatcher::watchLocks(bool watchCapsLock)
{
    set_modifiers(watchCapsLock ? modifier_caps_lock | modifier_num_lock : modifier_num_lock);
    set_flags(watch_flag_locked | watch_flag_unlocked);
    apply();
}

void TreelandKeyWatcher::treeland_keyboard_state_watcher_v1_state_changed(uint32_t modifier, uint32_t state)
{
    Q_EMIT stateChanged(modifier, state);
}

}
DS_END_NAMESPACE
