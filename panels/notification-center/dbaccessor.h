// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>

#include "notifyaccessor.h"

namespace notifycenter {
class NotifyEntity;
/**
 * @brief The DBAccessor class
 */
class DBAccessor : public DataAccessor
{
public:
    explicit DBAccessor();
    void open(const QString &dataPath);

    virtual NotifyEntity fetchEntity(const QString &id);
    virtual int fetchEntityCount(const QString &appName) const;
    virtual NotifyEntity fetchLastEntity(const QString &appName);
    virtual QList<NotifyEntity> fetchEntities(const QString &appName, int maxCount);
    virtual QList<QString> fetchApps(int maxCount) const;
    virtual void removeEntity(const QString &id);
    virtual void removeEntityByApp(const QString &appName);
    virtual void clear();

    virtual void addNotify(const QString &appName, const QString &content);

private:
    NotifyEntity parseEntity(const QSqlQuery &query);

private:
    QSqlDatabase m_connection;
};
}
