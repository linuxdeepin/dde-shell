// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QHash>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

#include <DConfig>
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <QQuickWindow>
#include <QWindow>

#include <csignal>
#include <iostream>

#include "applet.h"
#include "appletdata.h"
#include "containment.h"
#include "pluginloader.h"
#include "appletconfigmanager.h"
#include "appletloader.h"
#include "qmlengine.h"
#include "shell.h"

DS_USE_NAMESPACE
DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

DS_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(dsLog)
DS_END_NAMESPACE

void outputPluginTreeStruct(const DPluginMetaData &plugin, int level)
{
    const QString separator(level * 4, ' ');
    std::cout << qPrintable(separator + plugin.pluginId()) << std::endl;
    for (const auto &item : DPluginLoader::instance()->childrenPlugin(plugin.pluginId())) {
        outputPluginTreeStruct(item, level + 1);
    }
}

// disable log's output in some case.
static void disableLogOutput()
{
    QLoggingCategory::setFilterRules("*.debug=false");
}

class AppletManager
{
public:
    explicit AppletManager(const QStringList &pluginIds, AppletConfigManager *configManager)
    {
        qCDebug(dsLog) << "Preloading plugins:" << pluginIds;
        Q_ASSERT(configManager);

        m_configManager = configManager;
        Q_ASSERT(qobject_cast<DContainment *>(DPluginLoader::instance()->rootApplet()));

        m_dockConfig.reset(DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dock", QString(), qApp));
        if (m_dockConfig && m_dockConfig->isValid())
            m_showOnAllScreens = m_dockConfig->value(QStringLiteral("Show_On_All_Screens"), false).toBool();

        for (const auto &pluginId : pluginIds) {
            if (pluginId == dockPluginId() && m_showOnAllScreens) {
                for (auto *screen : qApp->screens())
                    addDockForScreen(screen);
            } else {
                createLoader(DAppletData{pluginId}, pluginIds);
            }
        }

        if (m_dockConfig && m_dockConfig->isValid()) {
            QObject::connect(m_dockConfig.data(), &DConfig::valueChanged, qApp, [this](const QString &key) {
                if (key == QStringLiteral("Show_On_All_Screens"))
                    setShowOnAllScreens(m_dockConfig->value(key, false).toBool());
            });
        }
        QObject::connect(qApp, &QGuiApplication::screenAdded, qApp, [this](QScreen *screen) {
            scheduleDockForScreen(screen);
        });
        QObject::connect(qApp, &QGuiApplication::screenRemoved, qApp, [this](QScreen *screen) {
            removeDockForScreen(screen);
        });

        QObject::connect(configManager,
                         &AppletConfigManager::appletEnabledChanged,
                         qApp,
                         [this](const QString &pluginId, bool enabled) {
                             auto loader = loaderForPlugin(pluginId);
                             if (!loader) {
                                 qCWarning(dsLog) << "Unable to find the loader for applet:" << pluginId;
                                 return;
                             }

                             if (enabled) {
                                 loader->setPluginId(pluginId);
                                 loader->exec();
                             } else {
                                 loader->remove(pluginId);
                             }
                         });
    }
    void enableSceneview()
    {
        auto rootApplet = qobject_cast<DContainment *>(DPluginLoader::instance()->rootApplet());
        rootApplet->setRootObject(DQmlEngine::createObject(QUrl("qrc:/shell/SceneWindow.qml")));
    }
    void exec()
    {
        for (auto loader : std::as_const(m_loaders)) {
            loader->exec();
        }
        m_started = true;
    }
    void quit()
    {
        for (auto item : std::as_const(m_loaders)) {
            item->deleteLater();
        }
    }

    DAppletLoader *loaderForPlugin(const QString &pluginId) const
    {
        QString rootPluginId = pluginId;
        QSet<QString> visitedPluginIds;
        while (!visitedPluginIds.contains(rootPluginId)) {
            visitedPluginIds.insert(rootPluginId);
            const auto parent = DPluginLoader::instance()->parentPlugin(rootPluginId);
            if (!parent.isValid())
                break;
            rootPluginId = parent.pluginId();
        }

        for (auto loader : m_loaders) {
            if (loader->applet() && loader->applet()->pluginId() == rootPluginId)
                return loader;
        }
        return nullptr;
    }

    QList<DAppletLoader *> m_loaders;

private:
    static QString dockPluginId() { return QStringLiteral("org.deepin.ds.dock"); }

    void scheduleDockForScreen(QScreen *screen, int attempt = 0)
    {
        if (!screen || !m_showOnAllScreens)
            return;

        QPointer<QScreen> pendingScreen(screen);
        QTimer::singleShot(250, qApp, [this, pendingScreen, attempt] {
            if (!pendingScreen || !m_showOnAllScreens || !qApp->screens().contains(pendingScreen))
                return;

            const auto geometry = pendingScreen->geometry();
            const bool geometryReady = geometry.width() > 1 && geometry.height() > 1;
            if (!geometryReady && attempt < 20) {
                scheduleDockForScreen(pendingScreen, attempt + 1);
                return;
            }

            if (geometryReady)
                addDockForScreen(pendingScreen);
        });
    }

    DAppletLoader *createLoader(const DAppletData &data, const QStringList &rootPluginIds)
    {
        auto rootApplet = qobject_cast<DContainment *>(DPluginLoader::instance()->rootApplet());
        auto applet = rootApplet->createApplet(data);
        if (!applet) {
            qCWarning(dsLog) << "Loading plugin failed:" << data.pluginId();
            return nullptr;
        }
        auto loader = new DAppletLoader(applet, m_configManager);
        m_loaders << loader;
        QObject::connect(loader, &DAppletLoader::failed, qApp, [this, loader, rootPluginIds](const QString &pluginId) {
            if (rootPluginIds.contains(pluginId)) {
                m_loaders.removeOne(loader);
                loader->deleteLater();
            }
        });
        return loader;
    }

    void addDockForScreen(QScreen *screen)
    {
        if (!screen || m_dockLoaders.contains(screen))
            return;
        QVariantMap data;
        data.insert(QStringLiteral("PluginId"), dockPluginId());
        data.insert(QStringLiteral("Id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
        data.insert(QStringLiteral("DockMirror"), true);
        data.insert(QStringLiteral("DockScreen"), screen->name());
        data.insert(QStringLiteral("DockMirrorPrimary"), screen == qApp->primaryScreen());
        if (auto *loader = createLoader(DAppletData(data), QStringList{dockPluginId()})) {
            m_dockLoaders.insert(screen, loader);
            if (m_started)
                loader->exec();
        }
    }

    void removeDockForScreen(QScreen *screen)
    {
        auto *loader = m_dockLoaders.take(screen);
        if (!loader)
            return;
        if (auto *applet = loader->applet()) {
            if (auto *rootApplet = qobject_cast<DContainment *>(applet->parentApplet()))
                rootApplet->removeApplet(applet);
        }
        m_loaders.removeOne(loader);
        loader->deleteLater();
    }

    void setShowOnAllScreens(bool enabled)
    {
        if (m_showOnAllScreens == enabled)
            return;
        m_showOnAllScreens = enabled;
        const auto screens = m_dockLoaders.keys();
        for (auto *screen : screens)
            removeDockForScreen(screen);
        if (m_showOnAllScreens) {
            for (auto *screen : qApp->screens())
                addDockForScreen(screen);
        } else {
            if (auto *loader = createLoader(DAppletData{dockPluginId()}, QStringList{dockPluginId()}))
                loader->exec();
        }
    }

    QHash<QScreen *, DAppletLoader *> m_dockLoaders;
    QScopedPointer<DConfig> m_dockConfig;
    AppletConfigManager *m_configManager = nullptr;
    bool m_showOnAllScreens = false;
    bool m_started = false;
};

int main(int argc, char *argv[])
{
    setenv("DSG_APP_ID", "org.deepin.dde.shell", 0);
    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::UseInactiveColorGroup, false);
    DApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    // Don't apply to plugins
    qunsetenv("QT_SCALE_FACTOR");
    // dde-shell contains UI controls based on QML and Widget technologies.
    // Due to the inconsistency of the default font rendering methods of different schemes,
    // the font effects are not uniform.
    // In order to ensure the same rendering effect, QML text rendering is changed to Native mode.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    a.setOrganizationName("deepin");
    a.setApplicationName("org.deepin.dde-shell");
    a.setDesktopFileName(QStringLiteral("org.deepin.dde-shell"));
    a.setApplicationVersion(QT_STRINGIFY(DS_VERSION));

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
    QCommandLineOption listOption("list", "List all applets.", QString());
    parser.addOption(listOption);
    QCommandLineOption sceneviewOption("sceneview", "View applets in scene, it only works without Window.", QString());
    parser.addOption(sceneviewOption);
    QCommandLineOption dbusServiceNameOption("serviceName", "Registed DBus service for the serviceName, if it's not empty.", "serviceName", QString("org.deepin.dde.shell"));
    parser.addOption(dbusServiceNameOption);

    parser.process(a);

    if (parser.isSet(listOption)) {
        disableLogOutput();
        for (const auto &item : DPluginLoader::instance()->rootPlugins()) {
            outputPluginTreeStruct(item, 0);
        }
        return 0;
    }

    auto dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QStringList fallbacks = QIcon::fallbackSearchPaths();
    for (const auto &fb : dirs) {
        fallbacks << fb + QLatin1String("/icons");
    }
    // To Fix (developer-center#8413) Qt6 find icons will ignore ${GenericDataLocation}/icons/xxx.png
    QIcon::setFallbackSearchPaths(fallbacks);

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();
    Dtk::Core::DLogManager::registerJournalAppender();
    qCInfo(dsLog) << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();

    Shell shell;
    QString serviceName(parser.value(dbusServiceNameOption));
    if (!parser.isSet(dbusServiceNameOption)) {
        const QString prefix(serviceName);
        if (parser.isSet(categoryOption)) {
            QString subfix(parser.value(categoryOption));
            serviceName = QString("%1.%2").arg(prefix).arg(subfix);
        } else {
            QString subfix(QString::number(QGuiApplication::applicationPid()));
            serviceName = QString("%1.random%2").arg(prefix).arg(subfix);
        }
    }
    if (!shell.registerDBusService(serviceName)) {
        qCFatal(dsLog).noquote() << QString("Can't start an instance for the dbus serviceName: \"%1\".").arg(serviceName);
        return -1;
    }
    qCInfo(dsLog).noquote() << QStringLiteral("Register dbus service for the serviceName: \"%1\"").arg(serviceName);

    QList<QString> pluginIds;
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
        pluginIds.removeIf([disabledApplets] (const QString &item) {
            return disabledApplets.contains(item);
        });
    }

    AppletConfigManager appletConfigManager;
    appletConfigManager.ensureAppletConfigs();
    // TODO disable qml's cache avoid to parsing error for ExecutionEngine.
    shell.disableQmlCache();
    shell.setFlickableWheelDeceleration(6000);

    AppletManager manager(pluginIds, &appletConfigManager);
    if (parser.isSet(sceneviewOption))
        manager.enableSceneview();

    QMetaObject::invokeMethod(&a, [&manager](){
        manager.exec();
    }, Qt::QueuedConnection);

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [&manager]() {
        qCInfo(dsLog) << "Exit dde-shell.";
        DPluginLoader::instance()->destroy();
        manager.quit();
    });

    return a.exec();
}
