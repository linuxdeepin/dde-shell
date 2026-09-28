// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "expiretimer.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QTimer>

#include <algorithm>

namespace notification {

Q_DECLARE_LOGGING_CATEGORY(notifyLog)

static const int DefaultTimeoutMSecs = 5000;

// Hover grace period: after the hover leaves, the countdown resumes with at
// least this much time left so the bubble lingers briefly.
static const int BlockItemTimeout = 1000;

// Expire timeout in milliseconds for entity: 0 means never expire (Critical
// urgency or expireTimeout 0) and -1 falls back to the 5000 ms default.
static int effectiveTimeout(const NotifyEntity &entity)
{
    if (entity.urgency() == NotifyEntity::Critical || entity.timeout() == 0)
        return 0;

    return entity.timeout() == -1 ? DefaultTimeoutMSecs : entity.timeout();
}

ExpireTimer::ExpireTimer(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ExpireTimer::onTimeout);
}

ExpireTimer *ExpireTimer::instance()
{
    static ExpireTimer expireTimer;
    return &expireTimer;
}

void ExpireTimer::push(const NotifyEntity &entity)
{
    const auto id = entity.id();
    const int interval = effectiveTimeout(entity);

    // The bubble and the staging area can show the same entity: keep the first
    // deadline instead of restarting its countdown.
    for (auto iter = m_pendingEntities.cbegin(); iter != m_pendingEntities.cend(); ++iter) {
        if (iter.value().id() == id && iter.value().cTime() == entity.cTime())
            return;
    }

    // This is the former NotificationManager::removePendingEntity replacement
    // path. A replacement occupies the old bubble slot, so remove that slot's
    // pending entry before starting the replacement's countdown.
    if (entity.isReplace()) {
        for (auto iter = m_pendingEntities.begin(); iter != m_pendingEntities.end(); ++iter) {
            if (iter.value().bubbleId() != entity.bubbleId())
                continue;

            const auto oldId = iter.value().id();
            m_pendingEntities.erase(iter);
            if (m_blockId == oldId)
                m_blockId = id;
            onTimeout();
            break;
        }
    }

    if (interval <= 0) {
        // Never expire: cancel any pending countdown for this id.
        remove(id);
        return;
    }

    const auto current = QDateTime::currentMSecsSinceEpoch();
    const auto point = current + interval;
    m_pendingEntities.insert(point, entity);

    if (m_lastPoint > point) {
        m_lastPoint = point;
        m_timer->start(static_cast<int>(qMax<qint64>(0, point - QDateTime::currentMSecsSinceEpoch())));
    }
}

void ExpireTimer::remove(qint64 id)
{
    // Stopped by id: the close paths only know the notification id, and an
    // entry that already expired on its own is gone.
    for (auto iter = m_pendingEntities.begin(); iter != m_pendingEntities.end(); ++iter) {
        if (iter.value().id() != id)
            continue;
        m_pendingEntities.erase(iter);
        onTimeout();
        return;
    }
}

void ExpireTimer::setBlockId(qint64 id)
{
    if (id == m_blockId)
        return;

    // The hover left the previously blocked id: give it at least the grace
    // period so its bubble lingers briefly after the hover ends.
    if (m_blockId != NotifyEntity::InvalidId) {
        const auto current = QDateTime::currentMSecsSinceEpoch();
        for (auto iter = m_pendingEntities.begin(); iter != m_pendingEntities.end(); ++iter) {
            if (iter.value().id() != m_blockId)
                continue;
            if (current > iter.key() - BlockItemTimeout) {
                const auto blockedEntity = iter.value();
                m_pendingEntities.erase(iter);
                m_pendingEntities.insert(current + BlockItemTimeout, blockedEntity);
            }
            break;
        }
    }

    m_blockId = id;
    onTimeout();
}

void ExpireTimer::onTimeout()
{
    QList<NotifyEntity> timeoutEntities;

    const auto current = QDateTime::currentMSecsSinceEpoch();
    for (auto iter = m_pendingEntities.begin(); iter != m_pendingEntities.end();) {
        if (iter.key() > current) {
            ++iter;
            continue;
        }
        timeoutEntities << iter.value();
        iter = m_pendingEntities.erase(iter);
    }

    for (const auto &item : timeoutEntities) {
        if (!item.isValid()) {
            qWarning(notifyLog) << "Skipping timeout processing for invalid entity id:" << item.id()
                                << "appName:" << item.appName() << "cTime:" << item.cTime();
            continue;
        }

        // A hovered notification must not expire: postpone it one grace period,
        // so the shared timer polls it at a low frequency instead of busy
        // looping at 0 ms.
        if (item.id() == m_blockId) {
            m_pendingEntities.insert(current + BlockItemTimeout, item);
            continue;
        }
        qDebug(notifyLog) << "Expired for the notification" << item.id() << item.appName();
        Q_EMIT expired(item.id(), item.bubbleId());
    }

    // Reschedule for the nearest remaining deadline.
    if (m_pendingEntities.isEmpty()) {
        m_timer->stop();
        m_lastPoint = std::numeric_limits<qint64>::max();
        return;
    }

    auto points = m_pendingEntities.keys();
    std::sort(points.begin(), points.end());
    m_lastPoint = points.first();
    m_timer->start(static_cast<int>(qMax<qint64>(0, m_lastPoint - QDateTime::currentMSecsSinceEpoch())));
}

}
