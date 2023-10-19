// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <DLog>
#include <DGuiApplicationHelper>

#include "applet.h"
#include "pluginloader.h"

DS_USE_NAMESPACE
DGUI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication a(argc, argv);
    a.setOrganizationName("deepin");
    a.setApplicationName("org.deepin.dde-shell");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption coronaOption("c", "collections of corona.", "corona", QString());
    parser.addOption(coronaOption);
    QCommandLineOption testOption(QStringList() << "t" << "test", "application test.");
    parser.addOption(testOption);

    parser.process(a);

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();
    Dtk::Core::DLogManager::registerJournalAppender();
    qInfo() << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();

    QList<QString> pluginIds;
    QList<DApplet *> applets;
    if (parser.isSet(testOption)) {
        pluginIds << "org.deepin.ds.corona-example";
    }
    if (parser.isSet(coronaOption)) {
        pluginIds << parser.values(coronaOption);
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
        applet->load();
        applet->init();
    }

    return a.exec();
}
