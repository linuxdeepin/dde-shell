// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationsetting.h"

#include <DConfig>

#include <QVariant>

namespace notification {

NotificationSetting::NotificationSetting(QObject *parent)
    : QObject(parent)
    , m_dConfig(Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.ds.notification"))
{

}

void NotificationSetting::initAllSettings()
{

}

void NotificationSetting::setAppSetting(const QString &id, NotificationSetting::AppConfigItem item)
{

}

QVariant NotificationSetting::getAppSetting(const QString &id, NotificationSetting::AppConfigItem item)
{
    return QVariant();
}

void NotificationSetting::setSystemSetting(NotificationSetting::SystemConfigItem item, const QVariant &value)
{
    switch (item) {
        case DNDMode:
            m_dConfig->setValue("dndmode", value);
            break;
        case OpenByTimeInterval:
            m_dConfig->setValue("open-by-time-interval", value);
            break;
        case LockScreenOpenDNDMode:
            m_dConfig->setValue("lockscreen-open-dndmode", value);
            break;
        case StartTime:
            m_dConfig->setValue("start-time", value);
            break;
        case EndTime:
            m_dConfig->setValue("end-time", value);
            break;
        case CloseNotification:
            m_dConfig->setValue("notification-closed", value);
            break;
        case MaxCount:
            m_dConfig->setValue("max-count", value);
            break;
        default:
            return;
    }
}

QVariant NotificationSetting::getSystemSetting(NotificationSetting::SystemConfigItem item)
{
    QVariant result;
    switch (item) {
        case DNDMode:
            result = m_dConfig->value("dndmode");
            break;
        case LockScreenOpenDNDMode:
            result = m_dConfig->value("lockscreen-open-dndmode");
            break;
        case OpenByTimeInterval:
            result = m_dConfig->value("open-by-time-interval");
            break;
        case StartTime:
            result = m_dConfig->value("start-time");
            break;
        case EndTime:
            result = m_dConfig->value("end-time");
            break;
        case CloseNotification:
            result = m_dConfig->value("notification-closed");
            break;
        case MaxCount:
            result = m_dConfig->value("max-count");
            break;
    }

    return result;
}

void NotificationSetting::appAdded(const QString &appId)
{

}

void NotificationSetting::appRemoved(const QString &id)
{

}

QStringList NotificationSetting::getAppLists()
{
    return QStringList();
}

} // notification