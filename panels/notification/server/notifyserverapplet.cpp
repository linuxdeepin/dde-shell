// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyserverapplet.h"
#include "notificationmanager.h"
#include "dbusadaptor.h"
#include "pluginfactory.h"

namespace notification {

Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notification")

NotifyServerApplet::NotifyServerApplet(QObject *parent)
        : DApplet(parent)
{

}

NotifyServerApplet::~NotifyServerApplet()
{
    qDebug(notificationLog) << "Exit notification server.";
    if (m_manager) {
        m_manager->deleteLater();
    }
    if (m_worker) {
        m_worker->exit();
        m_worker->wait();
        m_worker->deleteLater();
    }
}

bool NotifyServerApplet::load()
{
    return DApplet::load();
}

bool NotifyServerApplet::init()
{
    DApplet::init();

    m_manager = new NotificationManager();

    if (!m_manager->registerDbusService()) {
        qWarning(notificationLog) << QString("Can't register Notifications to the D-Bus object.");
        return false;
    }

    new DbusAdaptor(m_manager);
    new DDENotificationDbusAdaptor(m_manager);

    connect(m_manager, &NotificationManager::notificationStateChanged, this, &NotifyServerApplet::notificationStateChanged);

    m_worker = new QThread();
    m_manager->moveToThread(m_worker);
    m_worker->start();
    return true;
}

void NotifyServerApplet::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    m_manager->actionInvoked(id, bubbleId, actionKey);
}

void NotifyServerApplet::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    m_manager->notificationClosed(id, bubbleId, reason);
}

void NotifyServerApplet::notificationReplaced(qint64 id)
{
    m_manager->notificationReplaced(id);
}

QVariant NotifyServerApplet::appValue(const QString &appId, int configItem)
{
    return m_manager->GetAppInfo(appId, configItem);
}

void NotifyServerApplet::removeNotification(qint64 id)
{
    m_manager->removeNotification(id);
}

void NotifyServerApplet::removeNotifications(const QString &appName)
{
    m_manager->removeNotifications(appName);
}

void NotifyServerApplet::removeNotifications()
{
    m_manager->removeNotifications();
}

D_APPLET_CLASS(NotifyServerApplet)

}

#include "notifyserverapplet.moc"
