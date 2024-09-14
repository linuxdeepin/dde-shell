// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationcenterpanel.h"

#include "notifyaccessor.h"
#include "dbaccessor.h"
#include "pluginfactory.h"

#include <QLoggingCategory>

namespace notification {
namespace {
Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notificationcenter")
}

NotificationCenterPanel::NotificationCenterPanel(QObject *parent)
    : DPanel(parent)
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
    DPanel::init();

    auto accessor = new notifycenter::DBAccessor();
    notifycenter::NotifyAccessor::instance()->setDataAccessor(accessor);

    return true;
}

bool NotificationCenterPanel::visible() const
{
    return m_visible;
}

D_APPLET_CLASS(NotificationCenterPanel)
}

#include "notificationcenterpanel.moc"
