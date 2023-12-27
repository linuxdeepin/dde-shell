// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "dsglobal.h"
#include "docktray.h"
#include "pluginfactory.h"
#include "environments.h"

#include <QDir>
#include <QProcess>
#include <QCoreApplication>

DS_BEGIN_NAMESPACE
namespace dock {
DockTray::DockTray(QObject *parent)
    : DApplet(parent)
{
};

bool DockTray::load()
{
    DApplet::load();

    QObject::connect(this, &DApplet::rootObjectChanged, this, [this]() {
        if (rootObject()) {
            QStringList filters;
            filters << "*.so";

            for (auto pluginDir : pluginDirs) {
                QDir dir(pluginDir);
                auto plugins = dir.entryList(filters, QDir::Files);
                foreach(QString plugin, plugins) {
                    plugin = pluginDir + plugin;
        #ifdef QT_DEBUG
                    QProcess::startDetached(QString("%1/../panels/dock/tray/client/loader/dockplugin-loader").arg(qApp->applicationDirPath()), {"-p", plugin, "-platform", "wayland",});
        #else
                    QProcess::startDetached(QString("%1/%2/dockplugin-loader").arg(CMAKE_INSTALL_PREFIX).arg(CMAKE_INSTALL_LIBEXECDIR), {"-p", plugin, "-platform", "wayland"});
        #endif
                }
            }
        }
    });
    return true;
}
D_APPLET_CLASS(DockTray)
}

DS_END_NAMESPACE
#include "docktray.moc"
