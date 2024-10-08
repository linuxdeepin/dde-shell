// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationcenterpanel.h"

#include "notificationcenterproxy.h"
#include "notifyaccessor.h"
#include "common/dbaccessor.h"
#include "pluginfactory.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QLoggingCategory>

namespace notification {
Q_LOGGING_CATEGORY(notificationCenterLog, "dde.shell.notificationcenter")

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
    if (qEnvironmentVariableIntValue("DS_ENABLE_NOTIFICATIONCENTER"))
        return DPanel::load();

    return false;
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

    auto accessor = new notification::DBAccessor("Query");
    notifycenter::NotifyAccessor::instance()->setDataAccessor(accessor);

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
