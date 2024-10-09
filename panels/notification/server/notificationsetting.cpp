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
    switch (item) {
    case NotificationSetting::EnableNotification:
        return true;
    case NotificationSetting::ShowInNotificationCenter:
        return true;
    default:
        return QVariant();
    }
    return QVariant();
}

void NotificationSetting::setSystemSetting(NotificationSetting::SystemConfigItem item, const QVariant &value)
{
    switch (item) {
        case DNDMode:
            m_dConfig->setValue("dndMode", value);
            break;
        case OpenByTimeInterval:
            m_dConfig->setValue("openByTimeInterval", value);
            break;
        case LockScreenOpenDNDMode:
            m_dConfig->setValue("lockscreenOpenDndMode", value);
            break;
        case StartTime:
            m_dConfig->setValue("startTime", value);
            break;
        case EndTime:
            m_dConfig->setValue("endTime", value);
            break;
        case CloseNotification:
            m_dConfig->setValue("notificationClosed", value);
            break;
        case MaxCount:
            m_dConfig->setValue("maxCount", value);
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
            result = m_dConfig->value("dndMode");
            break;
        case LockScreenOpenDNDMode:
            result = m_dConfig->value("lockscreenOpenDndMode");
            break;
        case OpenByTimeInterval:
            result = m_dConfig->value("openByTimeInterval");
            break;
        case StartTime:
            result = m_dConfig->value("startTime");
            break;
        case EndTime:
            result = m_dConfig->value("endTime");
            break;
        case CloseNotification:
            result = m_dConfig->value("notificationClosed");
            break;
        case MaxCount:
            result = m_dConfig->value("maxCount");
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
