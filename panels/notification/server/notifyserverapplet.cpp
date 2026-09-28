// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyserverapplet.h"
#include "notificationmanager.h"
#include "dbusadaptor.h"
#include "expiretimer.h"
#include "pluginfactory.h"

#include <QThread>
#include <QLoggingCategory>

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}

namespace notification {

NotifyServerApplet::NotifyServerApplet(QObject *parent)
        : DApplet(parent)
{

}

NotifyServerApplet::~NotifyServerApplet()
{
    qDebug(notifyLog) << "Exit notification server.";
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
        qWarning(notifyLog) << QString("Can't register Notifications to the D-Bus object.");
        return false;
    }

    new DbusAdaptor(m_manager);
    new DDENotificationDbusAdaptor(m_manager);

    connect(m_manager, &NotificationManager::NotificationStateChanged, this, &NotifyServerApplet::notificationStateChanged);

    // ExpireTimer tracks the countdown of every shown notification (bubble and
    // staging area). When a deadline passes, this is the single place that tells
    // the server to close the notification; the frontend views only react to the
    // resulting NotificationStateChanged instead of closing on their own.
    connect(ExpireTimer::instance(), &ExpireTimer::expired, this, [this](qint64 id, uint bubbleId) {
        QMetaObject::invokeMethod(m_manager, "notificationClosed", Qt::QueuedConnection,
                                  Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, NotifyEntity::Expired));
    });

    removeExpiredNotifications();

    m_worker = new QThread();
    m_manager->moveToThread(m_worker);
    m_worker->start();
    return true;
}

void NotifyServerApplet::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::QueuedConnection, Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::actionInvoked(qint64 id, const QString &actionKey)
{
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::QueuedConnection, Q_ARG(qint64, id), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    // The close paths of both frontends (bubble button, staging area) land in
    // this main-thread slot, where ExpireTimer lives, so stop the countdown
    // here instead of listening to NotificationStateChanged. remove() is a
    // no-op when the entry already expired on its own (its deadline was
    // dropped when expired() was emitted); a countdown missed by a path that
    // does not pass through here (e.g. the D-Bus CloseNotification) is still
    // rejected by NotificationManager's processed guard when it fires later.
    ExpireTimer::instance()->remove(id);

    // The manager lives on the worker thread, so deliver the close to it there.
    QMetaObject::invokeMethod(m_manager, "notificationClosed", Qt::QueuedConnection, Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, reason));
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

void NotifyServerApplet::removeExpiredNotifications()
{
    m_manager->removeExpiredNotifications();
}

D_APPLET_CLASS(NotifyServerApplet)

}

#include "notifyserverapplet.moc"
