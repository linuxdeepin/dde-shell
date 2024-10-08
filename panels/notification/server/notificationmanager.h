// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QDBusContext>
#include <QDBusVariant>

#include "sessionmanager1interface.h"

using UserSessionManager = org::deepin::dde::SessionManager1;

namespace notification {

class NotifyEntity;
class DataAccessor;
class NotificationSetting;

class NotificationManager : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(uint recordCount READ recordCount NOTIFY RecordCountChanged)
public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager() override;

    enum ClosedReason {
        Expired = 1,
        Dismissed = 2,
        Closed = 3,
        Unknown = 4,
    };

public:
    bool registerDbusService();

    uint recordCount() const;
    void actionInvoked(qint64 id, uint bubbleId, const QString &actionKey);
    void notificationClosed(qint64 id, uint bubbleId, uint reason);
    void notificationReplaced(qint64 id);

Q_SIGNALS:
    // Standard Notifications dbus implementation
    void ActionInvoked(uint id, const QString &actionKey);
    void NotificationClosed(uint id, uint reason);
    void NotificationClosed2(uint id, uint reason);

    // Extra DBus APIs
    void RecordCountChanged(uint count);
    void AppInfoChanged(const QString &id, uint item, const QDBusVariant &value);
    void SystemInfoChanged(uint item, const QDBusVariant &value);
    void AppAdded(const QString &id);
    void AppRemoved(const QString &id);

signals:
    void needShowEntity(const QVariantMap &entityInfo);
    void needCloseEntity(uint id);

public Q_SLOTS:
    // Standard Notifications dbus implementation
    QStringList GetCapabilities();
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout);
    void CloseNotification(uint id);
    void GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion);

    // Extra DBus APIS
    QStringList GetAppList();
    void SetAppInfo(const QString &appId, uint configItem, const QDBusVariant &value);
    QDBusVariant GetAppInfo(const QString &appId, uint configItem);

    QString GetAppSetting(const QString &appName);
    void SetAppSetting(const QString &settings);

    void SetSystemInfo(uint configItem, const QDBusVariant &value);
    QDBusVariant GetSystemInfo(uint configItem);

private:
    bool isDoNotDisturb() const;
    void tryPlayNotificationSound(const QString &appName, bool dndMode,
                                  const QStringList &actions, const QVariantMap &hints) const;
    void tryRecordEntity(const QString &appId, NotifyEntity &entity);
    void emitRecordCountChanged();

private:
    uint m_replacesCount = 0;

    DataAccessor *m_persistence = nullptr;
    NotificationSetting *m_notificationSetting = nullptr;
    UserSessionManager *m_userSessionManager = nullptr;
};

} // notification
