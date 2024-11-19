// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shutdownapplet.h"
#include "pluginfactory.h"

#include <QDebug>
#include <QGuiApplication>

#include <DDBusSender>
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
    return true;
}

bool ShutdownApplet::requestShutdown()
{
    if (QStringLiteral("wayland") == QGuiApplication::platformName()) {
        qDebug() << "request treeland shutdown";
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
