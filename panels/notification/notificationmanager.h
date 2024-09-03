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

class BubbleModel;
class NotificationEntity;
class Persistence;
class NotificationSetting;
class NotificationManager : public QObject, public QDBusContext
{
    Q_OBJECT
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
    BubbleModel *bubbleModel() const { return m_bubbles; }
    bool registerDbusService();

    void invokeDefaultAction(int bubbleIndex);
    void invokeAction(int bubbleIndex, const QString &actionId);
    void close(int bubbleIndex);
    void delayProcess(int bubbleIndex);

Q_SIGNALS:
    // Standard Notifications dbus implementation
    void ActionInvoked(uint id, const QString &actionKey);
    void NotificationClosed(uint id, uint reason);

    // Extra DBus APIs
    void RecordAdded(const QString &id);
    void RecordCountChanged(uint count);
    void AppInfoChanged(const QString &id, uint item, const QDBusVariant &value);
    void SystemInfoChanged(uint item, const QDBusVariant &value);
    void AppAdded(const QString &id);
    void AppRemoved(const QString &id);

public Q_SLOTS:
    // Standard Notifications dbus implementation
    QStringList GetCapabilities();
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout);
    void CloseNotification(uint id);
    QString GetServerInformation(QString &name, QString &vendor, QString &specVersion);

    // Extra DBus APIS
    QStringList GetAppList();
    void SetAppInfo(const QString &appId, uint configItem, const QDBusVariant &value);
    QDBusVariant GetAppInfo(const QString &appId, uint configItem);

    QString GetAppSetting(const QString &appName);
    void SetAppSetting(const QString &settings);

    void SetSystemInfo(uint configItem, const QDBusVariant &value);
    QDBusVariant GetSystemInfo(uint configItem);

    // notification center
    void Toggle();
    void Show();
    void Hide();

private:
    void initConnections(NotificationEntity *entity);
    bool isDoNotDisturb() const;
    void tryPlayNotificationSound(const QString &appName, bool dndMode,
                                  const QStringList &actions, const QVariantMap &hints) const;
    void setReplaceId(NotificationEntity *entity);
    void tryRecordOne(const QString &appId, NotificationEntity *entity);

private slots:
    void bubbleExpired(NotificationEntity *entity);

private:
    int m_replaceCount = 0;

    Persistence *m_persistence = nullptr;
    NotificationSetting *m_notificationSetting = nullptr;
    BubbleModel *m_bubbles = nullptr;
    UserSessionManager *m_userSessionManager = nullptr;
};

} // notification
