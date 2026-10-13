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
        // Cancel model-thread callbacks before the worker can delete the setting.
        m_manager->stopAppMonitoring();
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

    // ExpireTimer runs on the main thread while the manager lives on the worker
    // thread, so the expiry close is forwarded through this typed signal with a
    // queued connection; the wrapper slots below keep their existing
    // QMetaObject::invokeMethod delivery.
    connect(this, &NotifyServerApplet::notificationClosedForwarded,
            m_manager, &NotificationManager::notificationClosed, Qt::QueuedConnection);

    // ExpireTimer tracks every displayed notification. On expiry, close it
    // through the same path as a user close; the frontends then close their
    // views from the resulting NotificationStateChanged.
    connect(ExpireTimer::instance(), &ExpireTimer::expired, this, [this](qint64 id, uint bubbleId) {
        Q_EMIT notificationClosedForwarded(id, bubbleId, NotifyEntity::Expired);
    });

    removeExpiredNotifications();

    m_worker = new QThread();
    m_manager->moveToThread(m_worker);
    m_worker->start();
    return true;
}

void NotifyServerApplet::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::QueuedConnection,
                              Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::actionInvoked(qint64 id, const QString &actionKey)
{
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::QueuedConnection,
                              Q_ARG(qint64, id), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    // Both frontend close paths (bubble button, notification center) land in
    // this main-thread slot, so stop the countdown here before queueing the
    // close to the manager. remove() is a no-op when the entry already expired
    // on its own; a close that bypasses this slot (D-Bus CloseNotification)
    // leaves its entry to fire later, where the manager drops it as stale.
    ExpireTimer::instance()->remove(id);

    // The manager lives on the worker thread, so deliver the close to it there.
    QMetaObject::invokeMethod(m_manager, "notificationClosed", Qt::QueuedConnection,
                              Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, reason));
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
