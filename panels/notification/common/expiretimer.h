// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMultiHash>
#include <QObject>

#include <limits>

#include "notifyentity.h"

class QTimer;

namespace notification {

/**
 * @brief Process-wide singleton that tracks the expire deadline of every
 *        displayed notification (bubble or staging area) with one shared
 *        single-shot QTimer.
 *
 * This is the notification server's pending-timeout machinery moved intact to
 * the presentation side: entities are stored in a QMultiHash keyed by absolute
 * deadline, the nearest deadline drives one single-shot QTimer, replacement
 * removes the old bubble slot, and one hovered id is blocked with a short grace
 * period after hover. The frontend-specific addition is idempotent push: when
 * the bubble and staging area present the same entity, they share the first
 * deadline instead of restarting it.
 */
class ExpireTimer : public QObject
{
    Q_OBJECT
public:
    static ExpireTimer *instance();

    // Starts the countdown of the entity's id based on its urgency and expire
    // timeout. A non-positive timeout (Critical urgency or expireTimeout 0)
    // cancels any pending countdown, i.e. the notification never expires.
    // Starting an already tracked id keeps the original deadline instead of
    // restarting it. A replacement notification (isReplace()) cancels the old
    // countdown of the same bubble slot before starting the new one.
    void push(const NotifyEntity &entity);
    // Stops the countdown of the notification id. The server is the single
    // owner of a notification's lifecycle: it calls this once the notification
    // is closed or archived, so the frontend views don't touch the timer on
    // their own. No-op when the id already expired on its own (its entry was
    // dropped when expired() was emitted).
    void remove(qint64 id);
    // Blocks the hovered id from expiring. Only one id is blocked at a time:
    // switching to another id (or passing InvalidId) unblocks the previous one,
    // which then expires with at least a short grace period left so its bubble
    // lingers briefly after the hover ends.
    void setBlockId(qint64 id);

Q_SIGNALS:
    // Emitted once when the deadline of id passes.
    void expired(qint64 id, uint bubbleId);

private:
    explicit ExpireTimer(QObject *parent = nullptr);

    void onTimeout();

    QTimer *m_timer = nullptr;
    // Pending entities keyed by their absolute expire deadline (ms since the
    // epoch). A multi hash so several entities can share a deadline point.
    QMultiHash<qint64, NotifyEntity> m_pendingEntities;
    // Nearest pending deadline; used to decide whether a new push must restart
    // the timer. InvalidId-like sentinel when nothing is pending.
    qint64 m_lastPoint = std::numeric_limits<qint64>::max();
    // The single hovered id that must not expire. InvalidId means no block.
    qint64 m_blockId = NotifyEntity::InvalidId;
};

}
