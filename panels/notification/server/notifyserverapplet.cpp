// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyserverapplet.h"
#include "notificationmanager.h"
#include "dbusadaptor.h"
#include "pluginfactory.h"

#include <QCoreApplication>
#include <QEvent>
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

    constexpr int kWaitTimeoutMs = 3000;
    constexpr int kTerminateWaitMs = 1000;

    if (m_worker) {
        if (m_worker->isRunning()) {
            m_worker->quit();

            if (!m_worker->wait(kWaitTimeoutMs)) {
                qWarning(notifyLog)
                    << "Worker thread did not exit in time, terminating.";

                m_worker->terminate();

                if (!m_worker->wait(kTerminateWaitMs)) {
                    qCritical(notifyLog)
                        << "Worker thread terminate timeout.";
                }
            }
        }

        if (m_manager) {
            delete m_manager;
            m_manager = nullptr;
        }

        delete m_worker;
        m_worker = nullptr;
    } else if (m_manager) {
        delete m_manager;
        m_manager = nullptr;
    }
}

bool NotifyServerApplet::load()
{
    return DApplet::load();
}

bool NotifyServerApplet::init()
{
    // Reentrancy protection
    if (m_manager || m_worker) {
        qWarning(notifyLog) << "NotifyServerApplet is already initialized.";
        return true;
    }

    DApplet::init();

    m_manager = new NotificationManager();

    if (!m_manager->registerDbusService()) {
        qWarning(notifyLog) << QString("Can't register Notifications to the D-Bus object.");

        delete m_manager;
        m_manager = nullptr;

        return false;
    }

    new DbusAdaptor(m_manager);
    new DDENotificationDbusAdaptor(m_manager);

    connect(m_manager, &NotificationManager::NotificationStateChanged, this, &NotifyServerApplet::notificationStateChanged);

    removeExpiredNotifications();

    m_worker = new QThread();
    m_manager->moveToThread(m_worker);

    // Register Qt asynchronous cleanup
    // connect(m_worker, &QThread::finished, m_manager, &QObject::deleteLater);
    // connect(m_worker, &QThread::finished, m_worker, &QObject::deleteLater);

    m_worker->start();
    return true;
}

void NotifyServerApplet::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    CHECK_MANAGER();
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::DirectConnection, Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::actionInvoked(qint64 id, const QString &actionKey)
{
    CHECK_MANAGER();
    QMetaObject::invokeMethod(m_manager, "actionInvoked", Qt::DirectConnection, Q_ARG(qint64, id), Q_ARG(QString, actionKey));
}

void NotifyServerApplet::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    CHECK_MANAGER();
    QMetaObject::invokeMethod(m_manager, "notificationClosed", Qt::DirectConnection, Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, reason));
}

QVariant NotifyServerApplet::appValue(const QString &appId, int configItem)
{
    CHECK_MANAGER_RET({});
    return m_manager->GetAppInfo(appId, configItem);
}

void NotifyServerApplet::removeNotification(qint64 id)
{
    CHECK_MANAGER();
    m_manager->removeNotification(id);
}

void NotifyServerApplet::removeNotifications(const QString &appName)
{
    CHECK_MANAGER();
    m_manager->removeNotifications(appName);
}

void NotifyServerApplet::removeNotifications()
{
    CHECK_MANAGER();
    m_manager->removeNotifications();
}

void NotifyServerApplet::removeExpiredNotifications()
{
    CHECK_MANAGER();
    m_manager->removeExpiredNotifications();
}

void NotifyServerApplet::setBlockClosedId(qint64 id)
{
    CHECK_MANAGER();
    m_manager->setBlockClosedId(id);
}

D_APPLET_CLASS(NotifyServerApplet)

}

#include "notifyserverapplet.moc"
