// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbaccessor.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>

#include "notifyentity.h"

namespace notifycenter {
namespace {
Q_LOGGING_CATEGORY(notifyLog, "notify.db")
}

static const QString TableName = "notifications";
static const QString TableName_v2 = "notifications2";
static const QString ColumnId = "ID";
static const QString ColumnIcon = "Icon";
static const QString ColumnSummary = "Summary";
static const QString ColumnBody = "Body";
static const QString ColumnAppName = "AppName";
static const QString ColumnCTime = "CTime";
static const QString ColumnAction = "Action";
static const QString ColumnHint = "Hint";
static const QString ColumnReplacesId = "ReplacesId";
static const QString ColumnTimeout = "Timeout";

static const QStringList EntityFields {
    ColumnId,
    ColumnIcon,
    ColumnSummary,
    ColumnBody,
    ColumnAppName,
    ColumnCTime,
    ColumnAction,
    ColumnHint
};

static QString notificationDBPath()
{
    QStringList dataPaths;
    if (qEnvironmentVariableIsSet("DS_NOTIFICATION_DB_PATH")) {
        auto path = qEnvironmentVariable("DS_NOTIFICATION_DB_PATH");
        dataPaths << path.split(";");
    }
    if (dataPaths.isEmpty()) {
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        QDir dir(dataDir);
        QString path = dir.absoluteFilePath("deepin/dde-osd/data.db");
        dataPaths << path;
    }
    QString dataPath;
    for (auto path : dataPaths) {
        if (QFileInfo::exists(path)) {
            return path;
            break;
        }
    }
    qWarning() << "Doesn't exist the data path" << dataPaths;
    return QString();
}

DBAccessor::DBAccessor()
{
    const auto dataPath = notificationDBPath();
    if (!dataPath.isEmpty()) {
        open(dataPath);
    }
}

void DBAccessor::open(const QString &dataPath)
{
    if (m_connection.isOpen()) {
        m_connection.close();
    }
    m_connection = QSqlDatabase::addDatabase("QSQLITE", "QSQLITE");
    m_connection.setDatabaseName(dataPath);
    qDebug(notifyLog) << "Open database path" << dataPath;
    if (!m_connection.open()) {
        qWarning() << "Open database error" << m_connection.lastError().text();
    }
}

NotifyEntity DBAccessor::fetchEntity(const QString &id)
{
    QSqlQuery query(m_connection);
    QString cmd = QString("SELECT %1 FROM notifications2 WHERE ID = :id").arg(EntityFields.join(","));
    query.prepare(cmd);
    query.bindValue(":id", id);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    if (query.next())
        return parseEntity(query);

    return {};
}

int DBAccessor::fetchEntityCount(const QString &appName) const
{
    QSqlQuery query(m_connection);
    if (!appName.isEmpty()) {
        QString cmd = QString("SELECT COUNT(*) FROM notifications2 WHERE AppName = :appName");
        query.prepare(cmd);
        query.bindValue(":appName", appName);
    } else {
        QString cmd = QString("SELECT COUNT(*) FROM notifications2");
        query.prepare(cmd);
    }

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    if (query.next())
        return query.value(0).toInt();

    return 0;
}

NotifyEntity DBAccessor::fetchLastEntity(const QString &appName)
{
    QSqlQuery query(m_connection);
    QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName ORDER BY CTime DESC LIMIT 1").arg(EntityFields.join(","));
    query.prepare(cmd);
    query.bindValue(":appName", appName);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    if (query.next()) {
        auto entity = parseEntity(query);

        qDebug(notifyLog) << "Fetched last entity" << entity.id();
        return entity;
    }
    return {};
}

QList<NotifyEntity> DBAccessor::fetchEntities(const QString &appName, int maxCount)
{
    QSqlQuery query(m_connection);
    if (maxCount >= 0) {
        QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName ORDER BY CTime DESC LIMIT :limit").arg(EntityFields.join(","));
        query.prepare(cmd);
        query.bindValue(":appName", appName);
        query.bindValue(":limit", maxCount);
    } else {
        QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName ORDER BY CTime DESC").arg(EntityFields.join(","));
        query.prepare(cmd);
        query.bindValue(":appName", appName);
    }

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    QList<NotifyEntity> ret;
    while (query.next()) {
        auto entity = parseEntity(query);
        if (!entity.isValid())
            continue;
        ret.append(entity);
    }

    qDebug(notifyLog) << "Fetched entities size:" << ret.size();
    return ret;
}

QList<QString> DBAccessor::fetchApps(int maxCount) const
{
    QSqlQuery query(m_connection);
    if (maxCount >= 0) {
        QString cmd("SELECT DISTINCT AppName FROM notifications2 ORDER BY CTime DESC LIMIT :limit");
        query.prepare(cmd);
        query.bindValue(":limit", maxCount);
    } else {
        QString cmd("SELECT DISTINCT AppName FROM notifications2 ORDER BY CTime DESC");
        query.prepare(cmd);
    }

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    QList<QString> ret;
    while (query.next()) {
        const auto name = query.value(0).toString();
        ret.append(name);
    }

    qDebug(notifyLog) << "Fetched apps count" << ret.size();

    return ret;
}

void DBAccessor::removeEntity(const QString &id)
{
    QSqlQuery query(m_connection);

    QString cmd("DELETE FROM notifications2 WHERE ID = :id");
    query.prepare(cmd);
    query.bindValue(":id", id);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return;
    }

    qDebug() << "Delete notify count" << query.numRowsAffected();
}

void DBAccessor::removeEntityByApp(const QString &appName)
{
    QSqlQuery query(m_connection);

    QString cmd("DELETE FROM notifications2 WHERE AppName = :appName");
    query.prepare(cmd);
    query.bindValue(":appName", appName);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return;
    }

    qDebug() << "Delete notify count" << query.numRowsAffected();
}

void DBAccessor::clear()
{
    QSqlQuery query(m_connection);

    QString cmd("DELETE FROM notifications2");
    query.prepare(cmd);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return;
    }

    qDebug() << "Delete notify count" << query.numRowsAffected();
}

void DBAccessor::addNotify(const QString &appName, const QString &content)
{
    QSqlQuery query(m_connection);

    QString cmd("INSERT INTO notifications2 (AppName, Body, CTime) VALUES (:appName, :body, :time)");
    query.prepare(cmd);
    query.bindValue(":appName", appName);
    query.bindValue(":body", content);
    query.bindValue(":time", QString::number(QDateTime::currentMSecsSinceEpoch()));

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return;
    }

    qDebug() << "Add notify count" << query.numRowsAffected();
}

NotifyEntity DBAccessor::parseEntity(const QSqlQuery &query)
{
    const auto id = query.value(0).toString();
    const auto icon = query.value(1).toString();
    const auto summary = query.value(2).toString();
    const auto body = query.value(3).toString();
    const auto name = query.value(4).toString();
    const auto time = query.value(5).toString();
    const auto action = query.value(6).toString();
    const auto hint = query.value(7).toString();

    NotifyEntity entity(id, name);
    entity.setIconName(icon);
    entity.setTitle(summary);
    entity.setContent(body);
    entity.setTime(time.toLongLong());
    entity.setHint(hint);
    entity.setAction(action);

    return entity;
}
}
