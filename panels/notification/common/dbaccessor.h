// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>

#include "dataaccessor.h"

namespace notification {
class NotifyEntity;

/**
 * @brief The DBAccessor class
 */
class DBAccessor : public DataAccessor
{
public:
    explicit DBAccessor();
    bool open(const QString &dataPath);

    qint64 addEntity(const NotifyEntity &entity) override;
    void updateEntityProcessedType(qint64 id, int processedType) override;

    NotifyEntity fetchEntity(const QString &id, int processedType) override;
    int fetchEntityCount(const QString &appName, int processedType) const override;
    NotifyEntity fetchLastEntity(const QString &appName, int processedType) override;
    QList<NotifyEntity> fetchEntities(const QString &appName, int maxCount, int processedType) override;
    QList<QString> fetchApps(int maxCount) const override;

    void removeEntity(qint64 id) override;
    void removeEntityByApp(const QString &appName) override;
    void clear() override;

private:
    void tryToCreateTable();

    bool isAttributeValid(const QString &tableName, const QString &attributeName);
    bool addAttributeToTable(const QString &tableName, const QString &attributeName, const QString &type);
    void updateProcessTypeValue();

private:
    NotifyEntity parseEntity(const QSqlQuery &query);

private:
    QSqlDatabase m_connection;
};
}
