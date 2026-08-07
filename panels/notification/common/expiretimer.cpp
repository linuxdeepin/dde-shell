// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "expiretimer.h"

#include <QDateTime>
#include <QTimer>

#include <algorithm>

#include "notifyentity.h"

namespace notification {

static const int DefaultTimeoutMSecs = 5000;

ExpireTimer::ExpireTimer(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ExpireTimer::onTimeout);
}

void ExpireTimer::start(qint64 key, int interval)
{
    if (interval <= 0)
        return;

    m_deadlines.insert(key, QDateTime::currentMSecsSinceEpoch() + interval);
    schedule();
}

void ExpireTimer::stop(qint64 key)
{
    if (m_deadlines.remove(key) > 0)
        schedule();
}

void ExpireTimer::stopAll()
{
    m_deadlines.clear();
    m_timer->stop();
}

bool ExpireTimer::contains(qint64 key) const
{
    return m_deadlines.contains(key);
}

int ExpireTimer::remaining(qint64 key) const
{
    const auto it = m_deadlines.constFind(key);
    if (it == m_deadlines.cend())
        return 0;

    return static_cast<int>(qMax<qint64>(0, it.value() - QDateTime::currentMSecsSinceEpoch()));
}

void ExpireTimer::schedule()
{
    if (m_deadlines.isEmpty()) {
        m_timer->stop();
        return;
    }

    auto it = std::min_element(m_deadlines.cbegin(), m_deadlines.cend(),
                               [](const qint64 &lhs, const qint64 &rhs) { return lhs < rhs; });
    const qint64 remaining = qMax<qint64>(0, it.value() - QDateTime::currentMSecsSinceEpoch());
    m_timer->start(static_cast<int>(remaining));
}

void ExpireTimer::onTimeout()
{
    const auto now = QDateTime::currentMSecsSinceEpoch();
    const QList<qint64> expiredKeys = [this, now] {
        QList<qint64> keys;
        for (auto it = m_deadlines.cbegin(); it != m_deadlines.cend(); ++it) {
            if (it.value() <= now)
                keys.append(it.key());
        }
        return keys;
    }();

    for (const auto &key : expiredKeys) {
        if (m_deadlines.remove(key) > 0)
            Q_EMIT expired(key);
    }

    schedule();
}

int effectiveTimeout(int urgency, int expireTimeout)
{
    // Critical notifications never expire.
    if (urgency == NotifyEntity::Critical)
        return 0;

    if (expireTimeout == 0)
        return 0;

    return expireTimeout == -1 ? DefaultTimeoutMSecs : expireTimeout;
}

}
