// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationcenterpanel.h"

#include "notificationcenterproxy.h"
#include "notifyaccessor.h"
#include "dbaccessor.h"

#include <pluginfactory.h>
#include <pluginloader.h>
#include <applet.h>
#include <containment.h>

#include <QQueue>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QLoggingCategory>

DS_USE_NAMESPACE

namespace notification {
Q_LOGGING_CATEGORY(notificationCenterLog, "dde.shell.notificationcenter")

static const QString DDENotifyDBusServer = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusInterface = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusPath = "/org/deepin/dde/Notification1";

static QDBusInterface notifyCenterInterface()
{
    return QDBusInterface(DDENotifyDBusServer, DDENotifyDBusPath, DDENotifyDBusInterface);
}
static QList<DApplet *> appletList(const QString &pluginId)
{
    QList<DApplet *> ret;
    auto rootApplet = DPluginLoader::instance()->rootApplet();
    auto root = qobject_cast<DContainment *>(rootApplet);

    QQueue<DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (applet->pluginId() == pluginId)
                ret << applet;
        }
    }
    return ret;
}

NotificationCenterPanel::NotificationCenterPanel(QObject *parent)
    : DPanel(parent)
    , m_proxy(new NotificationCenterProxy(this))
{
}

NotificationCenterPanel::~NotificationCenterPanel()
{
}

bool NotificationCenterPanel::load()
{
    return DPanel::load();
}

bool NotificationCenterPanel::init()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerObject("/org/deepin/dde/shell/notification/center",
                            "org.deepin.dde.shell.notification.center",
                            m_proxy,
                            QDBusConnection::ExportAllSlots)) {
        qWarning(notificationCenterLog) << QString("Can't register to the D-Bus object.");
        return false;
    }

    DPanel::init();

    auto accessor = notification::DBAccessor::instance();
    notifycenter::NotifyAccessor::instance()->setDataAccessor(accessor);

    auto applets = appletList("org.deepin.ds.notificationserver");
    bool valid = false;
    if (!applets.isEmpty()) {
        if (auto server = applets.first()) {
            valid = QObject::connect(server,
                                     SIGNAL(notificationStateChanged(qint64, int)),
                                     notifycenter::NotifyAccessor::instance(),
                                     SLOT(onReceivedRecordStateChanged(qint64, int)),
                                     Qt::QueuedConnection);
            notifycenter::NotifyAccessor::instance()->setDataUpdater(server);
        }
    } else {
        // old interface by dbus
        auto connection = QDBusConnection::sessionBus();
        valid = connection.connect(DDENotifyDBusServer, DDENotifyDBusPath, DDENotifyDBusInterface,
                                   "RecordAdded", this, SLOT(onReceivedRecord(const QString &)));
    }
    if (!valid) {
        qWarning() << "NotifyConnection is invalid, and can't receive RecordAdded signal.";
    }

    return true;
}

bool NotificationCenterPanel::visible() const
{
    return m_visible;
}

void NotificationCenterPanel::setVisible(bool newVisible)
{
    if (m_visible == newVisible)
        return;
    m_visible = newVisible;
    emit visibleChanged();
}

void NotificationCenterPanel::close()
{
    if (m_proxy) {
        m_proxy->Hide();
    }
}

D_APPLET_CLASS(NotificationCenterPanel)
}

#include "notificationcenterpanel.moc"
