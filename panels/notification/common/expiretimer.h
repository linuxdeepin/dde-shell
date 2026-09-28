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
 * @brief Process-wide singleton that tracks when every displayed notification
 *        (bubble or staging area) expires, using one shared single-shot QTimer.
 *
 * This is the server's former pending-timeout machinery moved to the display
 * side: deadlines are absolute (ms since the epoch) and stored in a
 * QMultiHash, the nearest one drives the timer, and a countdown only starts
 * once a notification is actually shown. push() is idempotent, so the two
 * views showing the same entity share one deadline; setBlockId() keeps a
 * hovered notification alive with a short grace period after the hover leaves.
 */
class ExpireTimer : public QObject
{
    Q_OBJECT
public:
    static ExpireTimer *instance();

    // Starts (or keeps) the countdown of entity based on its urgency and expire
    // timeout. A non-positive timeout (Critical urgency or expireTimeout 0)
    // means it never expires and cancels any pending countdown. An already
    // tracked id keeps its original deadline; a replacement (isReplace()) first
    // cancels the countdown of the bubble slot it takes over.
    void push(const NotifyEntity &entity);
    // Stops the countdown of id; called by the server when the notification is
    // closed or archived. A no-op when the entry already expired on its own, as
    // expired() drops it.
    void remove(qint64 id);
    // Keeps the hovered notification from expiring; only one id is blocked at a
    // time. Unblocking (another id or InvalidId) leaves the previous one at
    // least a short grace period, so its bubble lingers briefly after the hover
    // ends.
    void setBlockId(qint64 id);

Q_SIGNALS:
    // Emitted when the deadline of a displayed notification passes.
    void expired(qint64 id, uint bubbleId);

private:
    explicit ExpireTimer(QObject *parent = nullptr);

    void onTimeout();

    QTimer *m_timer = nullptr;
    // Pending entities keyed by absolute expire deadline (ms since the epoch);
    // a multi hash because several entities can share a deadline.
    QMultiHash<qint64, NotifyEntity> m_pendingEntities;
    // Nearest pending deadline, used to decide whether a push restarts the
    // timer; max while nothing is pending.
    qint64 m_lastPoint = std::numeric_limits<qint64>::max();
    // The single hovered id that must not expire; InvalidId when none.
    qint64 m_blockId = NotifyEntity::InvalidId;
};

}
