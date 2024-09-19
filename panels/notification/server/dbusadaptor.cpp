// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusadaptor.h"
#include "notificationmanager.h"

namespace notification {

DbusAdaptor::DbusAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
}

QStringList DbusAdaptor::GetCapabilities()
{
    auto manager = qobject_cast<NotificationManager *>(parent());
    if (!manager)
        return QStringList();

    return manager->GetCapabilities();
}

uint DbusAdaptor::Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                         const QString &body, const QStringList &actions, const QVariantMap &hints,
                         int expireTimeout)
{
    auto manager = qobject_cast<NotificationManager *>(parent());
    if (!manager)
        return 0;

    return manager->Notify(appName, replacesId, appIcon, summary, body, actions, hints, expireTimeout);
}

void DbusAdaptor::CloseNotification(uint id)
{
    auto manager = qobject_cast<NotificationManager *>(parent());
    if (!manager)
        return;

    manager->CloseNotification(id);
}

void DbusAdaptor::GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion)
{
    auto manager = qobject_cast<NotificationManager *>(parent());
    if (!manager)
        return;

    manager->GetServerInformation(name, vendor, version, specVersion);
}

DDENotificationDbusAdaptor::DDENotificationDbusAdaptor(QObject *parent)
    : DbusAdaptor(parent)
{
    setAutoRelaySignals(true);
}

QStringList DDENotificationDbusAdaptor::GetAppList()
{
    return QStringList();
}

QDBusVariant DDENotificationDbusAdaptor::GetAppInfo(const QString &appId, uint configItem)
{
    return QDBusVariant();
}

void DDENotificationDbusAdaptor::SetAppInfo(const QString &appId, uint configItem, const QDBusVariant &value)
{

}

QString DDENotificationDbusAdaptor::GetAppSetting(const QString &appName)
{
    return QString();
}

void DDENotificationDbusAdaptor::SetAppSetting(const QString &settings)
{

}

void DDENotificationDbusAdaptor::SetSystemInfo(uint configItem, const QDBusVariant &value)
{

}

QDBusVariant DDENotificationDbusAdaptor::GetSystemInfo(uint configItem)
{
    return QDBusVariant();
}

} // notification
