// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <sys/types.h>

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThreadPool>

namespace dock {

struct PackageCacheEntry {
    QString version;
    QString pakType;
    qint64 timestamp;
};

class LaunchDurationReporter : public QObject
{
    Q_OBJECT
public:
    explicit LaunchDurationReporter(QObject *parent = nullptr);
    ~LaunchDurationReporter() override;

    void reportWindowAppeared(const QString &desktopId, const QString &desktopSourcePath, pid_t pid);

private:
    void doReport(const QString &desktopId,
                  const QString &uniqueId,
                  const QString &launchType,
                  const QString &version,
                  const QString &pakType);

    QHash<QString, PackageCacheEntry> m_packageCache;
    QMutex m_cacheMutex;
    QThreadPool m_workerPool;
};

}
