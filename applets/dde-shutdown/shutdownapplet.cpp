// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shutdownapplet.h"
#include "treelandlockscreen.h"

#include <QDebug>
#include <QGuiApplication>

#include <DDBusSender>

#include <pluginfactory.h>
DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE
namespace shutdown {

ShutdownApplet::ShutdownApplet(QObject *parent)
    : DApplet(parent)
{
}

ShutdownApplet::~ShutdownApplet()
{
}

bool ShutdownApplet::load()
{
    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        m_lockscreen.reset(new TreeLandLockScreen);
    }
    return true;
}

bool ShutdownApplet::requestShutdown(const QString &type)
{
    qDebug() << "request shutdown:" << type;
    if (m_lockscreen) {
        if (type.isEmpty()) {
            // TODO: left mouse clicked, show shutdown page
            m_lockscreen->shutdown();
        } else if (type == QStringLiteral("Shutdown")) {
            m_lockscreen->shutdown();
        } else if (type == QStringLiteral("Lock")) {
            m_lockscreen->lock();
        } else if (type == QStringLiteral("SwitchUser")) {
            m_lockscreen->switchUser();
        } else if (type == QStringLiteral("UpdateAndShutdown") || type == QStringLiteral("UpdateAndReboot") ||
            type == QStringLiteral("Suspend") || type == QStringLiteral("Hibernate") ||
            type == QStringLiteral("Restart") || type == QStringLiteral("Logout")) {
            // TODO: implement these types
            m_lockscreen->shutdown();
        }
    } else {
        DDBusSender()
            .service("org.deepin.dde.ShutdownFront1")
            .interface("org.deepin.dde.ShutdownFront1")
            .path("/org/deepin/dde/ShutdownFront1")
            .method("Show")
            .call();
    }

    return true;
}

D_APPLET_CLASS(ShutdownApplet)
}
DS_END_NAMESPACE

#include "shutdownapplet.moc"
