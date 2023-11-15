// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <DLog>
#include <DGuiApplicationHelper>

#include <csignal>

#include "applet.h"
#include "pluginloader.h"

DS_USE_NAMESPACE
DGUI_USE_NAMESPACE

void outputPluginTreeStruct(const DPluginMetaData &plugin, int level)
{
    const QString separator(level * 4, ' ');
    qInfo() << qPrintable(separator + plugin.pluginId());
    for (auto item : DPluginLoader::instance()->childrenPlugin(plugin.pluginId())) {
        outputPluginTreeStruct(item, level + 1);
    }
}

static void exitApp(int signal)
{
    Q_UNUSED(signal);
    QCoreApplication::exit();
}

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    a.setOrganizationName("deepin");
    a.setApplicationName("org.deepin.dde-shell");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption panelOption("p", "collections of panel.", "panel", QString());
    parser.addOption(panelOption);
    QCommandLineOption testOption(QStringList() << "t" << "test", "application test.");
    parser.addOption(testOption);
    QCommandLineOption disableAppletOption("d", "disabled applet.", "disable-applet", QString());
    parser.addOption(disableAppletOption);
    parser.addPositionalArgument("list", "list all applet.");

    parser.process(a);

    const auto positions = parser.positionalArguments();

    if (positions.size() >= 1) {
        const auto subcommand = positions[0];
        if (subcommand == "list") {
            for (auto item : DPluginLoader::instance()->rootPlugins()) {
                outputPluginTreeStruct(item, 0);
            }
            return 0;
        }
    }

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();
    Dtk::Core::DLogManager::registerJournalAppender();
    qInfo() << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();

    // add signal handler, and call QCoreApplication::exit.
    std::signal(SIGINT, exitApp);
    std::signal(SIGABRT, exitApp);
    std::signal(SIGTERM, exitApp);
    std::signal(SIGKILL, exitApp);

    QList<QString> pluginIds;
    QList<DApplet *> applets;
    if (parser.isSet(testOption)) {
        pluginIds << "org.deepin.ds.example";
    } else if (parser.isSet(panelOption)) {
        pluginIds << parser.values(panelOption);
    } else {
        for (auto item : DPluginLoader::instance()->rootPlugins()) {
            pluginIds << item.pluginId();
        }
    }
    if (parser.isSet(disableAppletOption)) {
        const auto disabledApplets = parser.values(disableAppletOption);
        DPluginLoader::instance()->setDisabledApplets(disabledApplets);
    }

    qInfo() << "Loading plugin id" << pluginIds;
    for (auto pluginId : pluginIds) {
        auto applet = DPluginLoader::instance()->loadApplet(pluginId);
        if (!applet) {
            qWarning() << "Loading plugin failed:" << pluginId;
            continue;
        }
        applets << applet;
    }
    for (auto applet : applets) {
        if (!applet->load()) {
            qWarning() << "Plugin load failed:" << applet->pluginId();
            continue;
        }
        if (!applet->init()) {
            qWarning() << "Plugin init failed:" << applet->pluginId();
            continue;
        }
    }

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [applets]() {
        qInfo() << "Exit dde-shell.";
        for (auto item : applets) {
            item->deleteLater();
        }
    });

    return a.exec();
}
