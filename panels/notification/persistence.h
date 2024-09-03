// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>

namespace notification {

class NotificationEntity;
class Persistence
{
public:
    explicit Persistence();

    uint addOne(const NotificationEntity *entity);
    void addAll(const QList<NotificationEntity *> &entities);
    void removeOne(uint storageId);

private:
    void initDB();
    void tryToCreateTable();

    bool isAttributeValid(const QString &tableName, const QString &attributeName);
    bool addAttributeToTable(const QString &tableName, const QString &attributeName);
    QString convertMapToString(const QVariantMap &map);

private:
    QSqlDatabase m_dbConnection;
    QSqlQuery m_query;
};

} // notification

