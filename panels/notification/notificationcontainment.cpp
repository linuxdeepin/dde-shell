// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationcontainment.h"

#include <QLoggingCategory>

#include "pluginfactory.h"

DS_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notification")

NotificationContainment::NotificationContainment(QObject *parent)
    : DContainment(parent)
{

}

NotificationContainment::~NotificationContainment()
{
}

bool NotificationContainment::load()
{
    if (qEnvironmentVariableIntValue("DS_ENABLE_NOTIFICATION"))
        return DContainment::load();

    qDebug(notificationLog) << "Don't enable notification plugin, we can enable it by DS_ENABLE_NOTIFICATION=1";
    return false;
}

D_APPLET_CLASS(NotificationContainment)

DS_END_NAMESPACE

#include "notificationcontainment.moc"
