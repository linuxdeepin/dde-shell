// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbaccessor.h"
#include "notifyentity.h"

#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>

namespace notification {

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
static const QString ColumnProcessedType = "ProcessedType";
static const QString ColumnNotifyId = "NotifyId";
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
    ColumnHint,
    ColumnProcessedType,
    ColumnNotifyId,
    ColumnReplacesId,
    ColumnTimeout
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

    for (auto path : dataPaths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    qWarning() << "Doesn't exist the data path" << dataPaths;
    return QString();
}

DBAccessor::DBAccessor(const QString &key)
    : m_key(key)
{
    const auto dataPath = notificationDBPath();
    qInfo() << "Accessor's key:" << m_key << ", dataPath:" << dataPath;
    if (!dataPath.isEmpty() && open(dataPath)) {
        tryToCreateTable();
    }
}

DBAccessor *DBAccessor::instance()
{
    static DBAccessor *instance = nullptr;
    if (!instance) {
        instance = new DBAccessor("Default");
    }
    return instance;
}

bool DBAccessor::open(const QString &dataPath)
{
    m_connection = QSqlDatabase::addDatabase("QSQLITE", "QSQLITE" + m_key);
    m_connection.setDatabaseName(dataPath);
    qDebug(notifyLog) << "Open database path" << dataPath;

    if (!m_connection.open()) {
        qWarning() << "Open database error" << m_connection.lastError().text();
        return false;
    }

    return true;
}

qint64 DBAccessor::addEntity(const NotifyEntity &entity)
{
    QSqlQuery query(m_connection);

    QString sqlCmd =  QString("INSERT INTO %1 (").arg(TableName_v2);
    sqlCmd += ColumnIcon + ",";
    sqlCmd += ColumnSummary + ",";
    sqlCmd += ColumnBody + ",";
    sqlCmd += ColumnAppName + ",";
    sqlCmd += ColumnCTime + ",";
    sqlCmd += ColumnAction + ",";
    sqlCmd += ColumnHint + ",";
    sqlCmd += ColumnReplacesId + ",";
    sqlCmd += ColumnNotifyId + ",";
    sqlCmd += ColumnTimeout + ",";
    sqlCmd += ColumnProcessedType + ")";
    sqlCmd += "VALUES (:icon, :summary, :body, :appName, :ctime, :action, :hint, :replacesId, :notifyId, :timeout, :processedType)";

    query.prepare(sqlCmd);
    query.bindValue(":icon", entity.appIcon());
    query.bindValue(":summary", entity.summary());
    query.bindValue(":body", entity.body());
    query.bindValue(":appName", entity.appName());
    query.bindValue(":ctime", entity.cTime());
    query.bindValue(":action", entity.actionsString());
    query.bindValue(":hint", entity.hintsString());
    query.bindValue(":replacesId", entity.replacesId());
    query.bindValue(":notifyId", entity.bubbleId());
    query.bindValue(":timeout", entity.expiredTimeout());
    query.bindValue(":processedType", entity.processedType());

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "insert value to database failed: " << query.lastError().text() << entity.bubbleId() << entity.cTime();
        return 0;
    } else {
#ifdef QT_DEBUG
        qDebug() << "insert value done, time is:" << entity.cTime();
#endif
    }

    int storageId = 0;
    // to get entity's id in database
    if (!query.exec(QString("SELECT last_insert_rowid() FROM %1;").arg(TableName_v2))) {
        qWarning() << "get entity's id failed: " << query.lastError().text() << entity.bubbleId() << entity.cTime();
    } else {
        query.next();
        storageId = query.value(0).toInt();
#ifdef QT_DEBUG
        qDebug() << "get entity's id done:" << entity.bubbleId();
#endif
    }

    return storageId;
}

void DBAccessor::updateEntityProcessedType(qint64 id, int processedType)
{
    QSqlQuery query(m_connection);

    QString cmd = QString("UPDATE %1 SET ProcessedType = :processed WHERE ID = :id").arg(TableName_v2);
    query.prepare(cmd);
    query.bindValue(":id", id);
    query.bindValue(":processed", processedType);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "update processed type execution error:" << query.lastError().text();
    }
}

NotifyEntity DBAccessor::fetchEntity(qint64 id)
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

int DBAccessor::fetchEntityCount(const QString &appName, int processedType) const
{
    QSqlQuery query(m_connection);
    if (!appName.isEmpty()) {
        QString cmd = QString("SELECT COUNT(*) FROM notifications2 WHERE AppName = :appName AND (ProcessedType = :processedType OR ProcessedType IS NULL)");
        query.prepare(cmd);
        query.bindValue(":appName", appName);
    } else {
        QString cmd = QString("SELECT COUNT(*) FROM notifications2 WHERE (ProcessedType = :processedType OR ProcessedType IS NULL)");
        query.prepare(cmd);
    }

    query.bindValue(":processedType", processedType);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    if (query.next())
        return query.value(0).toInt();

    return 0;
}

NotifyEntity DBAccessor::fetchLastEntity(const QString &appName, int processedType)
{
    QSqlQuery query(m_connection);
    QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName AND (ProcessedType = :processedType OR ProcessedType IS NULL) ORDER BY CTime DESC LIMIT 1").arg(EntityFields.join(","));
    query.prepare(cmd);
    query.bindValue(":appName", appName);
    query.bindValue(":processedType", processedType);

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

QList<NotifyEntity> DBAccessor::fetchEntities(const QString &appName, int processedType, int maxCount)
{
    QSqlQuery query(m_connection);
    if (maxCount >= 0) {
        QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName AND (ProcessedType = :processedType OR ProcessedType IS NULL) ORDER BY CTime DESC LIMIT :limit").arg(EntityFields.join(","));
        query.prepare(cmd);
        query.bindValue(":appName", appName);
        query.bindValue(":limit", maxCount);
    } else {
        QString cmd = QString("SELECT %1 FROM notifications2 WHERE AppName = :appName AND (ProcessedType = :processedType OR ProcessedType IS NULL) ORDER BY CTime DESC").arg(EntityFields.join(","));
        query.prepare(cmd);
        query.bindValue(":appName", appName);
    }

    query.bindValue(":processedType", processedType);

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

NotifyEntity DBAccessor::fetchLastEntity(uint notifyId)
{
    QSqlQuery query(m_connection);
    QString cmd = QString("SELECT %1 FROM notifications2 WHERE notifyId = :notifyId ORDER BY CTime DESC LIMIT 1").arg(EntityFields.join(","));
    query.prepare(cmd);
    query.bindValue(":notifyId", notifyId);

    qDebug(notifyLog) << "Exec query" << query.lastQuery();
    if (!query.exec()) {
        qWarning() << "Query execution error:" << query.lastError().text();
        return {};
    }

    if (query.next()) {
        auto entity = parseEntity(query);

        qDebug(notifyLog) << "Fetched last entity " << entity.id() <<" by the notifyId" << notifyId;
        return entity;
    }
    return {};
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

void DBAccessor::removeEntity(qint64 id)
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

void DBAccessor::tryToCreateTable()
{
    QSqlQuery query(m_connection);

    QString sql = QString("CREATE TABLE IF NOT EXISTS %1("
                           "%2 INTEGER PRIMARY KEY AUTOINCREMENT,").arg(TableName_v2, ColumnId);
    sql += ColumnIcon + " TEXT,";
    sql += ColumnSummary + " TEXT,";
    sql += ColumnBody + " TEXT,";
    sql += ColumnAppName + " TEXT,";
    sql += ColumnCTime + " TEXT,";
    sql += ColumnAction + " TEXT,";
    sql += ColumnHint + " TEXT,";
    sql += ColumnReplacesId + " TEXT,";
    sql += ColumnNotifyId + " TEXT,";
    sql += ColumnTimeout + " TEXT,";
    sql += ColumnProcessedType + " INTEGER)";

    query.prepare(sql);

    if (!query.exec()) {
        qWarning() << "create table failed" << query.lastError().text();
    }

    // add new columns in history
    QMap<QString, QString> newColumns;
    newColumns[ColumnAction] = "TEXT";
    newColumns[ColumnHint] = "TEXT";
    newColumns[ColumnReplacesId] = "TEXT";
    newColumns[ColumnNotifyId] = "TEXT";
    newColumns[ColumnTimeout] = "TEXT";
    newColumns[ColumnProcessedType] = "INTEGER";

    for (auto it = newColumns.begin(); it != newColumns.end(); ++it) {
        if (!isAttributeValid(TableName_v2, it.key())) {
            addAttributeToTable(TableName_v2, it.key(), it.value());

            // update processed type value for new columns
            if (it.key() == ColumnProcessedType) {
                updateProcessTypeValue();
            }
        }
    }
}

bool DBAccessor::isAttributeValid(const QString &tableName, const QString &attributeName)
{
    QSqlQuery query(m_connection);

    QString sqlCmd = QString("SELECT * FROM SQLITE_MASTER WHERE TYPE='table' AND NAME='%1'").arg(tableName);
    if (query.exec(sqlCmd)) {
        if (query.next()) {
            sqlCmd = QString("SELECT * FROM %2").arg(tableName);
            if (query.exec(sqlCmd)) {
                QSqlRecord record = query.record();
                int index = record.indexOf(attributeName);
                if (index == -1) {
                    return false;
                } else {
                    return true;
                }
            } else {
                qDebug() << sqlCmd << ",lastError:" << query.lastError().text();
                return false;
            }
        } else { // table not exist
            return false;
        }
    } else { // sql error
        qDebug() << sqlCmd << ",lastError:" << query.lastError().text();
        return false;
    }
}

bool DBAccessor::addAttributeToTable(const QString &tableName, const QString &attributeName, const QString &type)
{
    QSqlQuery query(m_connection);

    QString sqlCmd = QString("alter table %1 add %2 %3").arg(tableName, attributeName, type);
    if (query.exec(sqlCmd)) {
        return true;
    }

    return false;
}

void DBAccessor::updateProcessTypeValue()
{
    QSqlQuery query(m_connection);

    QString updateCmd = QString("UPDATE %1 SET ProcessedType = %2 WHERE ProcessedType IS NULL")
            .arg(TableName_v2, NotifyEntity::processedValue());

    if (!query.exec(updateCmd)) {
        qWarning() << "Failed to update ProcessedType NULL values:" << query.lastError();
    }
}

NotifyEntity DBAccessor::parseEntity(const QSqlQuery &query)
{
    const auto id = query.value(ColumnId).toLongLong();
    const auto icon = query.value(ColumnIcon).toString();
    const auto summary = query.value(ColumnSummary).toString();
    const auto body = query.value(ColumnBody).toString();
    const auto appName = query.value(ColumnAppName).toString();
    const auto time = query.value(ColumnCTime).toString();
    const auto action = query.value(ColumnAction).toString();
    const auto hint = query.value(ColumnHint).toString();
    const auto processedType = query.value(ColumnProcessedType).toUInt();
    const auto notifyId = query.value(ColumnNotifyId).toUInt();
    const auto replacesId = query.value(ColumnReplacesId).toUInt();
    const auto timeout = query.value(ColumnTimeout).toInt();

    NotifyEntity entity(id, appName);
    entity.setAppIcon(icon);
    entity.setSummary(summary);
    entity.setBody(body);
    entity.setCTime(time.toLongLong());
    entity.setHintString(hint);
    entity.setActionString(action);
    entity.setProcessedType(processedType);
    entity.setBubbleId(id);
    entity.setReplacesId(replacesId);
    entity.setExpiredTimeout(timeout);

    return entity;
}

}
