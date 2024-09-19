// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyserverapplet.h"
#include "notificationmanager.h"
#include "dbusadaptor.h"
#include "pluginfactory.h"
#include "notifyentity.h"

namespace notification {

Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notification")

NotifyServerApplet::NotifyServerApplet(QObject *parent)
        : DApplet(parent)
{

}

bool NotifyServerApplet::load()
{
    return DApplet::load();
}

bool NotifyServerApplet::init()
{
    DApplet::init();

    m_manager = new NotificationManager(this);
    if (!m_manager->registerDbusService()) {
        qWarning(notificationLog) << QString("Can't register Notifications to the D-Bus object.");
        return false;
    }

    new DbusAdaptor(m_manager);
    new DDENotificationDbusAdaptor(m_manager);

    connect(m_manager, &NotificationManager::needShowEntity, this, &NotifyServerApplet::needShowEntity);
    connect(m_manager, &NotificationManager::needCloseEntity, this, &NotifyServerApplet::needCloseEntity);
    return true;
}

void NotifyServerApplet::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    m_manager->actionInvoked(id, bubbleId, actionKey);

    Q_EMIT notificationStateChanged(id, NotifyEntity::processedValue());
}

void NotifyServerApplet::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    m_manager->notificationClosed(id, bubbleId, reason);

    Q_EMIT notificationStateChanged(id, NotifyEntity::processedValue());
}

void NotifyServerApplet::notificationReplaced(qint64 id)
{
    m_manager->notificationReplaced(id);

    Q_EMIT notificationStateChanged(id, NotifyEntity::removedValue());
}

D_APPLET_CLASS(NotifyServerApplet)

}

#include "notifyserverapplet.moc"
