// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>

namespace Dtk::Core {
    class DConfig;
}

namespace notification {

class NotificationSetting : public QObject
{
    Q_OBJECT

public:
    enum AppConfigItem {
        AppName,
        AppIcon,
        EnableNotification,
        EnablePreview,
        EnableSound,
        ShowInNotificationCenter,
        LockScreenShowNotification,
        ShowOnTop
    };

    enum SystemConfigItem {
        DNDMode,
        LockScreenOpenDNDMode,
        OpenByTimeInterval,
        StartTime,
        EndTime,
        CloseNotification,
        MaxCount
    };

public:
    explicit NotificationSetting(QObject *parent = nullptr);

    void initAllSettings();

    void setAppSetting(const QString &id, AppConfigItem item);
    QVariant getAppSetting(const QString &id, AppConfigItem item);

    void setSystemSetting(SystemConfigItem item, const QVariant &value);
    QVariant getSystemSetting(SystemConfigItem item);

    void appAdded(const QString &appId);
    void appRemoved(const QString &id);

    QStringList getAppLists();

private:
    Dtk::Core::DConfig *m_dConfig;
};

} // notification
