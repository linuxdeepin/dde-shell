// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shutdownapplet.h"
#include "treelandlockscreen.h"

#include <QDebug>
#include <QProcess>
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

bool ShutdownApplet::requestShutdown()
{
    return requestShutdown({});
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
        if (type == QStringLiteral("Lock")) {
            x11LockScreen();
        } else {
            QString method = type.isEmpty() ? "Show" : type;
            DDBusSender()
            .service("org.deepin.dde.ShutdownFront1")
            .interface("org.deepin.dde.ShutdownFront1")
            .path("/org/deepin/dde/ShutdownFront1")
            .method(method)
            .call();
        }
    }

    return true;
}

void ShutdownApplet::x11LockScreen()
{
    QProcess process;
    QString originMap;

    // Step 1: get current keyboard options
    process.start("/usr/bin/setxkbmap", QStringList() << "-query");
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        // 查找包含 "option" 的行（原 grep option 命令）
        if (line.contains("option")) {
            // 模拟 awk -F ' ' '{print $2}'
            // 按空格分割，跳过空字符串
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                originMap = parts[1];
                // 如果第二个字段以冒号结尾，可能需要进一步处理
                if (originMap.endsWith(':')) {
                    originMap = originMap.left(originMap.length() - 1);
                }
            }
            break;
        }
    }

    // Step 2: set keyboard options to un grab
    process.start("/usr/bin/setxkbmap", {"-option", "grab:break_actions"});
    if (!process.waitForFinished()) {
        qWarning() << "Failed to set keyboard options!";
    }

    // Step 3: analog keys xf86ungrab
    process.start("/usr/bin/xdotool", {"key", "XF86Ungrab"});
    if (!process.waitForFinished()) {
        qWarning() << "Failed to simulate XF86Ungrab key!";
    }

    // Step 4: call the lock screen via dbus
    process.start("dbus-send", {
        "--print-reply",
        "--dest=org.deepin.dde.LockFront1",
        "/org/deepin/dde/LockFront1",
        "org.deepin.dde.LockFront1.Show"
    });
    process.waitForFinished();

    // Step 5: restore original keyboard options
    if (!originMap.isEmpty()) {
        process.start("/usr/bin/setxkbmap", {"-option", originMap});
        process.waitForFinished();
    }
}

D_APPLET_CLASS(ShutdownApplet)
}
DS_END_NAMESPACE

#include "shutdownapplet.moc"
