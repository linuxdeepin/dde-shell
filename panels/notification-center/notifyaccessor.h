// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include "notifyentity.h"

class QQmlEngine;
class QJSEngine;
namespace notifycenter {
class DataAccessor {
public:
    virtual NotifyEntity fetchEntity(const QString &id) { Q_UNUSED(id); return {}; }
    virtual int fetchEntityCount(const QString &appName) const { Q_UNUSED(appName); return 0; }
    virtual NotifyEntity fetchLastEntity(const QString &appName) { Q_UNUSED(appName); return {}; }
    virtual QList<NotifyEntity> fetchEntities(const QString &appName, int maxCount)
    {
        Q_UNUSED(appName)
        Q_UNUSED(maxCount)
        return {};
    }
    virtual QList<QString> fetchApps(int maxCount) const { Q_UNUSED(maxCount); return {}; }
    virtual void removeEntity(const QString &id) { Q_UNUSED(id); }
    virtual void removeEntityByApp(const QString &appName) { Q_UNUSED(appName); }
    virtual void clear() {}

    // test
    virtual void addNotify(const QString &appName, const QString &content)
    {
        Q_UNUSED(appName)
        Q_UNUSED(content)
    }
};

/**
 * @brief The NotifyAccessor class
 */
class NotifyAccessor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString dataInfo READ dataInfo NOTIFY dataInfoChanged FINAL)
    Q_PROPERTY(QStringList apps READ apps NOTIFY appsChanged FINAL)
    Q_PROPERTY(bool debuging READ debuging NOTIFY debugingChanged)
public:
    static NotifyAccessor *instance();
    static NotifyAccessor *create(QQmlEngine *, QJSEngine *);

    void setDataAccessor(DataAccessor *accessor);

    void invokeAction(const NotifyEntity &entity, const QString &actionId);
    void pinApplication(const QString &appName, bool pin);
    bool applicationPin(const QString &appName) const;
    Q_INVOKABLE void openNotificationSetting();

    NotifyEntity fetchEntity(const QString &id) const;
    int fetchEntityCount(const QString &appName = QString()) const;
    NotifyEntity fetchLastEntity(const QString &appName) const;
    QList<NotifyEntity> fetchEntities(const QString &appName, int maxCount = -1);
    QStringList fetchApps(int maxCount = -1) const;
    void removeEntity(const QString &id);
    void removeEntityByApp(const QString &appName);
    void clear();

signals:
    void entityReceived(const QString &id);

public slots:
    void addNotify(const QString &appName, const QString &content);

signals:
    void dataInfoChanged();
    void appsChanged();
    void debugingChanged();

private slots:
    void onReceivedRecord(const QString &id);

private:
    explicit NotifyAccessor(QObject *parent = nullptr);

    void tryEmitAppsChanged(const QString &appName);
    QString dataInfo() const;
    QStringList apps() const;
    bool debuging() const;

private:
    DataAccessor *m_accessor = nullptr;
    QMap<QString, bool> m_pinnedApps;
    QStringList m_apps;
    bool m_debuging = false;
};
}
