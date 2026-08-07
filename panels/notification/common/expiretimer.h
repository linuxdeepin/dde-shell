// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>

class QTimer;

namespace notification {

/**
 * @brief Tracks expire deadlines for a set of keys using one shared QTimer.
 *
 * start() records a deadline per key and the nearest deadline drives a
 * single-shot QTimer. When a deadline passes, expired() is emitted once for
 * that key. All bookkeeping lives in hash maps, so no QTimer is allocated per
 * key and nothing leaks when a key expires or is stopped.
 */
class ExpireTimer : public QObject
{
    Q_OBJECT
public:
    explicit ExpireTimer(QObject *parent = nullptr);

    // Starts a countdown for key. A non-positive interval is ignored.
    void start(qint64 key, int interval);
    // Removes key so it never expires.
    void stop(qint64 key);
    // Removes every pending deadline and stops the timer.
    void stopAll();

    bool contains(qint64 key) const;
    // Milliseconds left for key, or 0 when it is not tracked.
    int remaining(qint64 key) const;

Q_SIGNALS:
    // Emitted once when the deadline of key passes.
    void expired(qint64 key);

private:
    void schedule();
    void onTimeout();

    QTimer *m_timer = nullptr;
    QHash<qint64, qint64> m_deadlines;
};

// Effective expire timeout in milliseconds for a notification.
// Returns 0 for "never expire" (Critical urgency or expireTimeout == 0) and
// falls back to the server default of 5000 ms for expireTimeout == -1.
int effectiveTimeout(int urgency, int expireTimeout);

}
