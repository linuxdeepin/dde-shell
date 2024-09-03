// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "persistence.h"
#include "notificationentity.h"

#include <QDir>
#include <QSqlError>
#include <QSqlTableModel>
#include <QStandardPaths>
#include <QSqlRecord>

namespace notification {

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

#define ACTION_SEGMENT ("|")
#define HINT_SEGMENT ("|")
#define KEYVALUE_SEGMENT ("!!!")

Persistence::Persistence()
{

}

uint Persistence::addOne(const NotificationEntity *entity)
{
    QString sqlCmd =  QString("INSERT INTO %1 (").arg(TableName_v2);
    sqlCmd += ColumnIcon + ",";
    sqlCmd += ColumnSummary + ",";
    sqlCmd += ColumnBody + ",";
    sqlCmd += ColumnAppName + ",";
    sqlCmd += ColumnCTime + ",";
    sqlCmd += ColumnAction + ",";
    sqlCmd += ColumnHint + ",";
    sqlCmd += ColumnReplacesId + ",";
    sqlCmd += ColumnTimeout + ")";
    sqlCmd += "VALUES (:icon, :summary, :body, :appname, :ctime, :action, :hint, :replacesid, :timeout)";

    m_query.prepare(sqlCmd);
    m_query.bindValue(":icon", entity->originIconName());
    m_query.bindValue(":summary", entity->title());
    m_query.bindValue(":body", entity->text());
    m_query.bindValue(":appname", entity->appName());
    m_query.bindValue(":ctime", entity->ctime());

    //action
    QString action;
            foreach (QString text, entity->actions()) {
            action += text;
            action += ACTION_SEGMENT;
        }
    if (!action.isEmpty())
        action = action.mid(0, action.length() - 1);
    m_query.bindValue(":action", action);

    //hint
    m_query.bindValue(":hint", convertMapToString(entity->hints()));
    m_query.bindValue(":replacesid", entity->replacesId());
    m_query.bindValue(":timeout", entity->timeout());

    if (!m_query.exec()) {
        qWarning() << "insert value to database failed: " << m_query.lastError().text() << entity->id() << entity->ctime();
        return 0;
    } else {
#ifdef QT_DEBUG
        qDebug() << "insert value done, time is:" << entity->ctime();
#endif
    }

    int rowId = 1;
    // to get entity's id in database
    if (!m_query.exec(QString("SELECT last_insert_rowid() FROM %1;").arg(TableName_v2))) {
        qWarning() << "get entity's id failed: " << m_query.lastError().text() << entity->id() << entity->ctime();
    } else {
        m_query.next();
        rowId = m_query.value(0).toInt(); // todo 是否需要
#ifdef QT_DEBUG
        qDebug() << "get entity's id done:" << entity->id();
#endif
    }

    return rowId;
}

void Persistence::addAll(const QList<NotificationEntity *> &entities)
{
    for (auto &entity : entities) {
        addOne(entity);
    }
}

void Persistence::initDB()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(dataDir);
    }

    m_dbConnection = QSqlDatabase::addDatabase("QSQLITE", "QSQLITE");
    m_dbConnection.setDatabaseName(dataDir + "/" + "data.db");
    if (!m_dbConnection.open()) {
        qWarning() << "open database error" << m_dbConnection.lastError().text();
    } else {
#ifdef QT_DEBUG
        qDebug() << "database open";
#endif
    }

    m_query = QSqlQuery(m_dbConnection);
    m_query.setForwardOnly(true);

    tryToCreateTable();

    QSqlTableModel tableModel(nullptr, m_dbConnection);
    tableModel.setTable(TableName_v2);
    tableModel.select();

    while (tableModel.canFetchMore())
        tableModel.fetchMore();
}

void Persistence::tryToCreateTable()
{
    QString text = QString("CREATE TABLE IF NOT EXISTS %1("
                           "%2 INTEGER PRIMARY KEY   AUTOINCREMENT,").arg(TableName_v2, ColumnId);
    text += ColumnIcon + " TEXT,";
    text += ColumnSummary + " TEXT,";
    text += ColumnBody + " TEXT,";
    text += ColumnAppName + " TEXT,";
    text += ColumnCTime + " TEXT,";
    text += ColumnAction + " TEXT,";
    text += ColumnHint + " TEXT,";
    text += ColumnReplacesId + " TEXT,";
    text += ColumnTimeout + " TEXT)";

    m_query.prepare(text);

    if (!m_query.exec()) {
        qWarning() << "create table failed" << m_query.lastError().text();
    }

    if (!isAttributeValid(TableName_v2, ColumnAction)) {
        addAttributeToTable(TableName_v2, ColumnAction);
    }

    if (!isAttributeValid(TableName_v2, ColumnHint)) {
        addAttributeToTable(TableName_v2, ColumnHint);
    }

    if (!isAttributeValid(TableName_v2, ColumnReplacesId)) {
        addAttributeToTable(TableName_v2, ColumnReplacesId);
    }

    if (!isAttributeValid(TableName_v2, ColumnTimeout)) {
        addAttributeToTable(TableName_v2, ColumnTimeout);
    }
}

bool Persistence::isAttributeValid(const QString &tableName, const QString &attributeName)
{
    QString sqlCmd = QString("SELECT * FROM SQLITE_MASTER WHERE TYPE='table' AND NAME='%1'").arg(tableName);
    if (m_query.exec(sqlCmd)) {
        if (m_query.next()) {
            sqlCmd = QString("SELECT * FROM %2").arg(tableName);
            if (m_query.exec(sqlCmd)) {
                QSqlRecord record = m_query.record();
                int index = record.indexOf(attributeName);
                if (index == -1) {
                    return false;
                } else {
                    return true;
                }
            } else {
                qDebug() << sqlCmd << ",lastError:" << m_query.lastError().text();
                return false;
            }
        } else { //table not exist
            return false;
        }
    } else {//sql error
        qDebug() << sqlCmd << ",lastError:" << m_query.lastError().text();
        return false;
    }
}

bool Persistence::addAttributeToTable(const QString &tableName, const QString &attributeName)
{
    QString sqlCmd = QString("alter table %1 add %2 TEXT").arg(tableName, attributeName);
    if (m_query.exec(sqlCmd)) {
        return true;
    }

    return false;
}

QString Persistence::convertMapToString(const QVariantMap &map)
{
    QString text;

    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        QString key = it.key();
        text += key;
        text += KEYVALUE_SEGMENT;
        QString value = it.value().toString();
        text += value;
        text += HINT_SEGMENT;
    }

    return text;
}

void Persistence::removeOne(uint storageId)
{
    m_query.prepare(QString("DELETE FROM %1 WHERE ID = (:id)").arg(TableName_v2));
    m_query.bindValue(":id", storageId);

    if (!m_query.exec()) {
        qWarning() << "remove value:" << storageId << "from database failed: " << m_query.lastError().text();
        return;
    } else {
#ifdef QT_DEBUG
        qDebug() << "db remove value:" << storageId;
#endif
    }
}

} // notification
