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
    return manager()->GetCapabilities();
}

uint DbusAdaptor::Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                         const QString &body, const QStringList &actions, const QVariantMap &hints,
                         int expireTimeout)
{
    return manager()->Notify(appName, replacesId, appIcon, summary, body, actions, hints, expireTimeout);
}

void DbusAdaptor::CloseNotification(uint id)
{
    manager()->CloseNotification(id);
}

void DbusAdaptor::GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion)
{
    manager()->GetServerInformation(name, vendor, version, specVersion);
}

NotificationManager *DbusAdaptor::manager() const
{
    return qobject_cast<NotificationManager *>(parent());
}

DDENotificationDbusAdaptor::DDENotificationDbusAdaptor(QObject *parent)
    : DbusAdaptor(parent)
{
    setAutoRelaySignals(true);
}

uint DDENotificationDbusAdaptor::recordCount() const
{
    return manager()->recordCount();
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
