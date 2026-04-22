// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <DPlatformTheme>
#include <QQuickWindow>
#include <QWindow>

#include <csignal>
#include <iostream>

#include "applet.h"
#include "containment.h"
#include "pluginloader.h"
#include "appletloader.h"
#include "qmlengine.h"
#include "shell.h"

DS_USE_NAMESPACE
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

static QString readAppearanceStringProperty(const QString &propertyName)
{
    static constexpr auto kAppearanceService = "org.deepin.dde.Appearance1";
    static constexpr auto kAppearancePath = "/org/deepin/dde/Appearance1";
    static constexpr auto kAppearanceInterface = "org.deepin.dde.Appearance1";
    static constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties";

    QDBusInterface appearanceProperties(QString::fromLatin1(kAppearanceService),
                                        QString::fromLatin1(kAppearancePath),
                                        QString::fromLatin1(kPropertiesInterface),
                                        QDBusConnection::sessionBus());
    if (!appearanceProperties.isValid()) {
        return QString();
    }

    const QDBusReply<QDBusVariant> reply = appearanceProperties.call(QStringLiteral("Get"),
                                                                     QString::fromLatin1(kAppearanceInterface),
                                                                     propertyName);
    return reply.isValid() ? reply.value().variant().toString() : QString();
}

static double readAppearanceDoubleProperty(const QString &propertyName)
{
    static constexpr auto kAppearanceService = "org.deepin.dde.Appearance1";
    static constexpr auto kAppearancePath = "/org/deepin/dde/Appearance1";
    static constexpr auto kAppearanceInterface = "org.deepin.dde.Appearance1";
    static constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties";

    QDBusInterface appearanceProperties(QString::fromLatin1(kAppearanceService),
                                        QString::fromLatin1(kAppearancePath),
                                        QString::fromLatin1(kPropertiesInterface),
                                        QDBusConnection::sessionBus());
    if (!appearanceProperties.isValid()) {
        return 0.0;
    }

    const QDBusReply<QDBusVariant> reply = appearanceProperties.call(QStringLiteral("Get"),
                                                                     QString::fromLatin1(kAppearanceInterface),
                                                                     propertyName);
    return reply.isValid() ? reply.value().variant().toDouble() : 0.0;
}

static DGuiApplicationHelper::ColorType explicitThemeTypeFromName(const QString &themeName)
{
    const QString normalizedThemeName = themeName.trimmed().toLower();
    if (normalizedThemeName.isEmpty()) {
        return DGuiApplicationHelper::UnknownType;
    }

    if (normalizedThemeName == QStringLiteral("dark")
        || normalizedThemeName.endsWith(QStringLiteral(".dark"))
        || normalizedThemeName.endsWith(QStringLiteral("-dark"))
        || normalizedThemeName.endsWith(QStringLiteral("_dark"))) {
        return DGuiApplicationHelper::DarkType;
    }

    if (normalizedThemeName == QStringLiteral("light")
        || normalizedThemeName.endsWith(QStringLiteral(".light"))
        || normalizedThemeName.endsWith(QStringLiteral("-light"))
        || normalizedThemeName.endsWith(QStringLiteral("_light"))) {
        return DGuiApplicationHelper::LightType;
    }

    return DGuiApplicationHelper::UnknownType;
}

static DGuiApplicationHelper::ColorType currentEffectiveThemeType(DGuiApplicationHelper *guiHelper)
{
    if (!guiHelper) {
        return DGuiApplicationHelper::LightType;
    }

    const auto currentThemeType = guiHelper->themeType();
    if (currentThemeType != DGuiApplicationHelper::UnknownType) {
        return currentThemeType;
    }

    const auto currentPaletteType = guiHelper->paletteType();
    if (currentPaletteType != DGuiApplicationHelper::UnknownType) {
        return currentPaletteType;
    }

    return DGuiApplicationHelper::LightType;
}

static DGuiApplicationHelper::ColorType effectiveThemeTypeFromAppearance(const QString &globalThemeName,
                                                                         const QString &gtkThemeName,
                                                                         const QString &iconThemeName,
                                                                         DGuiApplicationHelper *guiHelper)
{
    const auto globalThemeType = explicitThemeTypeFromName(globalThemeName);
    if (globalThemeType != DGuiApplicationHelper::UnknownType) {
        return globalThemeType;
    }

    const auto gtkThemeType = explicitThemeTypeFromName(gtkThemeName);
    if (gtkThemeType != DGuiApplicationHelper::UnknownType) {
        return gtkThemeType;
    }

    const auto iconThemeType = explicitThemeTypeFromName(iconThemeName);
    if (iconThemeType != DGuiApplicationHelper::UnknownType) {
        return iconThemeType;
    }

    return currentEffectiveThemeType(guiHelper);
}

static void syncApplicationThemeFromAppearance()
{
    auto *guiHelper = DGuiApplicationHelper::instance();
    if (!guiHelper) {
        return;
    }

    const QString globalThemeName = readAppearanceStringProperty(QStringLiteral("GlobalTheme"));
    const QString gtkThemeName = readAppearanceStringProperty(QStringLiteral("GtkTheme"));
    const QString iconThemeName = readAppearanceStringProperty(QStringLiteral("IconTheme"));
    const QString standardFontName = readAppearanceStringProperty(QStringLiteral("StandardFont"));
    const double fontSize = readAppearanceDoubleProperty(QStringLiteral("FontSize"));
    const auto explicitThemeType = explicitThemeTypeFromName(globalThemeName);
    const bool followSystemTheme = explicitThemeType == DGuiApplicationHelper::UnknownType;
    const auto targetPaletteType = followSystemTheme ? DGuiApplicationHelper::UnknownType : explicitThemeType;

    if (auto *applicationTheme = guiHelper->applicationTheme()) {
        const QByteArray gtkThemeNameBytes = gtkThemeName.toUtf8();
        if (!gtkThemeNameBytes.isEmpty() && applicationTheme->themeName() != gtkThemeNameBytes) {
            applicationTheme->setThemeName(gtkThemeNameBytes);
        }

        const QByteArray iconThemeNameBytes = iconThemeName.toUtf8();
        if (!iconThemeNameBytes.isEmpty() && applicationTheme->iconThemeName() != iconThemeNameBytes) {
            applicationTheme->setIconThemeName(iconThemeNameBytes);
        }

        const QByteArray standardFontNameBytes = standardFontName.toUtf8();
        if (!standardFontNameBytes.isEmpty() && applicationTheme->fontName() != standardFontNameBytes) {
            applicationTheme->setFontName(standardFontNameBytes);
        }

        if (fontSize > 0.0 && !qFuzzyCompare(applicationTheme->fontPointSize() + 1.0, fontSize + 1.0)) {
            applicationTheme->setFontPointSize(fontSize);
        }
    }

    if (guiHelper->paletteType() != targetPaletteType) {
        guiHelper->setPaletteType(targetPaletteType);
    }

    const auto targetThemeType = effectiveThemeTypeFromAppearance(globalThemeName, gtkThemeName, iconThemeName, guiHelper);
    const QPalette targetPalette = guiHelper->applicationPalette(targetThemeType);
    if (guiHelper->applicationPalette() != targetPalette) {
        guiHelper->setApplicationPalette(targetPalette);
    }

    if (!iconThemeName.isEmpty() && QIcon::themeName() != iconThemeName) {
        QIcon::setThemeName(iconThemeName);
    }

    if (qApp) {
        QFont appFont = qApp->font();
        bool fontChanged = false;

        if (!standardFontName.isEmpty() && appFont.family() != standardFontName) {
            appFont.setFamily(standardFontName);
            fontChanged = true;
        }

        if (fontSize > 0.0 && !qFuzzyCompare(appFont.pointSizeF() + 1.0, fontSize + 1.0)) {
            appFont.setPointSizeF(fontSize);
            fontChanged = true;
        }

        if (fontChanged) {
            qApp->setFont(appFont);
        }
    }
}

class NativeMenuThemeSync final : public QObject
{
public:
    explicit NativeMenuThemeSync(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void install()
    {
        qApp->installEventFilter(this);

        auto *helper = DGuiApplicationHelper::instance();
        if (!helper) {
            return;
        }

        QObject::connect(helper, &DGuiApplicationHelper::applicationPaletteChanged,
                         this, &NativeMenuThemeSync::syncAllMenus);
        QObject::connect(helper, &DGuiApplicationHelper::paletteTypeChanged,
                         this, &NativeMenuThemeSync::syncAllMenus);
        if (auto *platformTheme = helper->applicationTheme()) {
            QObject::connect(platformTheme, &DPlatformTheme::themeNameChanged,
                             this, &NativeMenuThemeSync::syncAllMenus);
            QObject::connect(platformTheme, &DPlatformTheme::fontNameChanged,
                             this, &NativeMenuThemeSync::syncAllMenus);
            QObject::connect(platformTheme, &DPlatformTheme::fontPointSizeChanged,
                             this, &NativeMenuThemeSync::syncAllMenus);
        }
    }

    void syncAllMenus()
    {
        const auto widgets = QApplication::allWidgets();
        for (QWidget *widget : widgets) {
            if (auto *menu = qobject_cast<QMenu *>(widget)) {
                syncMenu(menu);
            }
        }
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto *menu = qobject_cast<QMenu *>(watched);
        if (!menu) {
            return QObject::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::Polish:
        case QEvent::PolishRequest:
        case QEvent::PaletteChange:
        case QEvent::ApplicationFontChange:
        case QEvent::FontChange:
        case QEvent::Show:
            syncMenu(menu);
            break;
        default:
            break;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void syncMenu(QMenu *menu)
    {
        auto *helper = DGuiApplicationHelper::instance();
        if (!helper || !menu) {
            return;
        }

        auto paletteType = helper->paletteType();
        if (paletteType == DGuiApplicationHelper::UnknownType) {
            paletteType = helper->themeType();
        }

        if (qApp && menu->font() != qApp->font()) {
            menu->setFont(qApp->font());
        }
        menu->setPalette(helper->applicationPalette(paletteType));
        menu->ensurePolished();
        if (auto *style = menu->style()) {
            style->polish(menu);
        }
        menu->update();
    }
};

class AppearanceThemeSync final : public QObject
{
    Q_OBJECT

public:
    explicit AppearanceThemeSync(NativeMenuThemeSync *menuThemeSync, QObject *parent = nullptr)
        : QObject(parent)
        , m_menuThemeSync(menuThemeSync)
    {
        m_syncTimer.setSingleShot(true);
        m_syncTimer.setInterval(80);
        QObject::connect(&m_syncTimer, &QTimer::timeout,
                         this, &AppearanceThemeSync::refreshAppearanceState);

        m_pollTimer.setInterval(500);
        QObject::connect(&m_pollTimer, &QTimer::timeout,
                         this, &AppearanceThemeSync::refreshAppearanceState);
    }

    void install()
    {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(QStringLiteral("org.deepin.dde.Appearance1"),
                    QStringLiteral("/org/deepin/dde/Appearance1"),
                    QStringLiteral("org.deepin.dde.Appearance1"),
                    QStringLiteral("Changed"),
                    this,
                    SLOT(onAppearanceChanged(QString,QString)));
        bus.connect(QStringLiteral("org.deepin.dde.Appearance1"),
                    QStringLiteral("/org/deepin/dde/Appearance1"),
                    QStringLiteral("org.deepin.dde.Appearance1"),
                    QStringLiteral("Refreshed"),
                    this,
                    SLOT(onAppearanceRefreshed(QString)));
        bus.connect(QStringLiteral("org.deepin.dde.Appearance1"),
                    QStringLiteral("/org/deepin/dde/Appearance1"),
                    QStringLiteral("org.freedesktop.DBus.Properties"),
                    QStringLiteral("PropertiesChanged"),
                    QStringLiteral("sa{sv}as"),
                    this,
                    SLOT(onAppearancePropertiesChanged(QString,QVariantMap,QStringList)));

        refreshAppearanceState();
        m_pollTimer.start();
    }

private Q_SLOTS:
    void onAppearanceChanged(const QString &, const QString &)
    {
        scheduleSync();
    }

    void onAppearanceRefreshed(const QString &)
    {
        scheduleSync();
    }

    void onAppearancePropertiesChanged(const QString &interfaceName,
                                       const QVariantMap &,
                                       const QStringList &)
    {
        if (interfaceName != QStringLiteral("org.deepin.dde.Appearance1")) {
            return;
        }

        scheduleSync();
    }

    void refreshAppearanceState()
    {
        const QString globalThemeName = readAppearanceStringProperty(QStringLiteral("GlobalTheme"));
        const QString gtkThemeName = readAppearanceStringProperty(QStringLiteral("GtkTheme"));
        const QString iconThemeName = readAppearanceStringProperty(QStringLiteral("IconTheme"));
        const QString activeColor = readAppearanceStringProperty(QStringLiteral("QtActiveColor"));

        if (globalThemeName.isEmpty() && gtkThemeName.isEmpty() && iconThemeName.isEmpty()) {
            return;
        }

        syncApplicationThemeFromAppearance();

        if (globalThemeName != m_lastGlobalThemeName
            || gtkThemeName != m_lastGtkThemeName
            || iconThemeName != m_lastIconThemeName
            || activeColor != m_lastActiveColor) {
            m_lastGlobalThemeName = globalThemeName;
            m_lastGtkThemeName = gtkThemeName;
            m_lastIconThemeName = iconThemeName;
            m_lastActiveColor = activeColor;
            if (m_menuThemeSync) {
                m_menuThemeSync->syncAllMenus();
            }
        }
    }

private:
    void scheduleSync()
    {
        m_syncTimer.start();
    }

    QPointer<NativeMenuThemeSync> m_menuThemeSync;
    QTimer m_syncTimer;
    QTimer m_pollTimer;
    QString m_lastGlobalThemeName;
    QString m_lastGtkThemeName;
    QString m_lastIconThemeName;
    QString m_lastActiveColor;
};

class AppletManager
{
public:
    explicit AppletManager(const QStringList &pluginIds)
    {
        qCDebug(dsLog) << "Preloading plugins:" << pluginIds;
        auto rootApplet = qobject_cast<DContainment *>(DPluginLoader::instance()->rootApplet());
        Q_ASSERT(rootApplet);

        for (const auto &pluginId : pluginIds) {
            auto applet = rootApplet->createApplet(DAppletData{pluginId});
            if (!applet) {
                qCWarning(dsLog) << "Loading plugin failed:" << pluginId;
                continue;
            }

            auto loader = new DAppletLoader(applet);
            m_loaders << loader;

            QObject::connect(loader, &DAppletLoader::failed, qApp, [this, loader, pluginIds](const QString &pluginId) {
                if (pluginIds.contains(pluginId)) {
                    m_loaders.removeOne(loader);
                    loader->deleteLater();
                }
            });
        }
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
    }
    void quit()
    {
        for (auto item : std::as_const(m_loaders)) {
            item->deleteLater();
        }
    }
    QList<DAppletLoader *> m_loaders;
};

int main(int argc, char *argv[])
{
    setenv("DSG_APP_ID", "org.deepin.dde.shell", 0);
    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::UseInactiveColorGroup, false);
    DApplication a(argc, argv);
    syncApplicationThemeFromAppearance();
    auto *nativeMenuThemeSync = new NativeMenuThemeSync(&a);
    nativeMenuThemeSync->install();
    nativeMenuThemeSync->syncAllMenus();
    auto *appearanceThemeSync = new AppearanceThemeSync(nativeMenuThemeSync, &a);
    appearanceThemeSync->install();
    // Don't apply to plugins
    qunsetenv("QT_SCALE_FACTOR");
    // dde-shell contains UI controls based on QML and Widget technologies.
    // Due to the inconsistency of the default font rendering methods of different schemes,
    // the font effects are not uniform.
    // In order to ensure the same rendering effect, QML text rendering is changed to Native mode.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    a.setOrganizationName("deepin");
    a.setApplicationName("org.deepin.dde-shell");
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

    shell.dconfigsMigrate();
    // TODO disable qml's cache avoid to parsing error for ExecutionEngine.
    shell.disableQmlCache();
    shell.setFlickableWheelDeceleration(6000);

    AppletManager manager(pluginIds);
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

#include "main.moc"
