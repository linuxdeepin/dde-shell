// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <DLog>
#include <DGuiApplicationHelper>

#include <csignal>
#include <iostream>

#include "applet.h"
#include "pluginloader.h"
#include "appletloader.h"

DS_USE_NAMESPACE
DGUI_USE_NAMESPACE

DS_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(dsLog)
DS_END_NAMESPACE

void outputPluginTreeStruct(const DPluginMetaData &plugin, int level)
{
    const QString separator(level * 4, ' ');
    std::cout << qPrintable(separator + plugin.pluginId()) << std::endl;
    for (auto item : DPluginLoader::instance()->childrenPlugin(plugin.pluginId())) {
        outputPluginTreeStruct(item, level + 1);
    }
}

static void exitApp(int signal)
{
    Q_UNUSED(signal);
    QCoreApplication::exit();
}

class AppletManager
{
public:
    explicit AppletManager(const QList<DApplet *> &applets)
    {
        for (auto applet : std::as_const(applets)) {
            auto loader = new DAppletLoader(applet);
            m_loaders << loader;

            QObject::connect(loader, &DAppletLoader::failed, qApp, [this, loader]() {
                m_loaders.removeOne(loader);
                loader->deleteLater();
            });
        }
    }
    void exec()
    {
        for (auto loader : std::as_const(m_loaders)) {
            loader->exec();
        }
    }
    void quit()
    {
        for (auto item : std::as_const(m_loaders)) {
            if (auto applet = item->applet()) {
                applet->deleteLater();
                item->deleteLater();
            }
        }
    }
    QList<DAppletLoader *> m_loaders;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("deepin");
    a.setApplicationName("org.deepin.dde-shell");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption panelOption("p", "collections of panel.", "panel", QString());
    parser.addOption(panelOption);
    QCommandLineOption categoryOption("C", "collections of root panels by category.", "category", QString());
    parser.addOption(categoryOption);
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
    qCInfo(dsLog) << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();

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
    } else if (parser.isSet(categoryOption)) {
        const QList<QString> &categories = parser.values(categoryOption);
        for (const auto &item : DPluginLoader::instance()->rootPlugins()) {
            const auto catetroy = item.value("Category").toString();
            if (catetroy.isEmpty())
                continue;
            if (categories.contains(catetroy)) {
                pluginIds << item.pluginId();
            }
        }

    } else {
        for (const auto &item : DPluginLoader::instance()->rootPlugins()) {
            pluginIds << item.pluginId();
        }
    }
    if (parser.isSet(disableAppletOption)) {
        const auto disabledApplets = parser.values(disableAppletOption);
        DPluginLoader::instance()->setDisabledApplets(disabledApplets);
    }

    qCInfo(dsLog) << "Loading plugin id" << pluginIds;
    for (const auto &pluginId : pluginIds) {
        auto applet = DPluginLoader::instance()->loadApplet(DAppletData{pluginId});
        if (!applet) {
            qCWarning(dsLog) << "Loading plugin failed:" << pluginId;
            continue;
        }
        applets << applet;
    }

    AppletManager manager(applets);
    manager.exec();

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [&manager]() {
        qCInfo(dsLog) << "Exit dde-shell.";
        manager.quit();
    });

    return a.exec();
}
