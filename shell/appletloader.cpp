// SPDX-FileCopyrightText: 2023 -2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appletloader.h"
#include "appletconfigmanager.h"
#include "pluginloader.h"
#include "applet.h"
#include "containment.h"
#include "appletdata.h"
#include "qmlengine.h"

#include <dobject_p.h>

#include <QMap>
#include <QLoggingCategory>
#include <QElapsedTimer>
#include <QTranslator>
#include <QApplication>
#include <QFile>
#include <DWindowManagerHelper>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE;
DGUI_USE_NAMESPACE

Q_LOGGING_CATEGORY(dsLoaderLog, "org.deepin.dde.shell.loader")

namespace {
    class Benchmark
    {
    public:
        explicit Benchmark(const QString &text)
            : m_text(text)
        {
            m_timer.start();
        }
        ~Benchmark()
        {
            const auto elasped = m_timer.elapsed();
            if (elasped >= m_timeout) {
                qCWarning(dsLoaderLog) << qPrintable(m_text) << ": elasped time [" << elasped << "].";
            }
        }
    private:
        QString m_text;
        int m_timeout = 100;
        QElapsedTimer m_timer;
    };
}

class DAppletLoaderPrivate : public DObjectPrivate
{
public:
    explicit DAppletLoaderPrivate(DAppletLoader *qq)
        : DTK_CORE_NAMESPACE::DObjectPrivate(qq)
    {
    }
    QList<DAppletData> groupList(DApplet *applet, const DAppletData &data) const
    {
        if (!data.groupList().isEmpty())
            return data.groupList();

        QList<DAppletData> groups;
        const auto children = DPluginLoader::instance()->childrenPlugin(applet->pluginMetaData().pluginId());
        for (const auto &item : children) {
            groups << DAppletData::fromPluginMetaData(item);
        }
        return groups;
    }

    ~DAppletLoaderPrivate()
    {
        for (const auto &tl : std::as_const(m_pluginTranslators)) {
            tl->deleteLater();
        }
    }

    bool doLoad(DApplet *applet);
    void doCreateRootObject(DApplet *applet);
    bool doInit(DApplet *applet);

    bool load(DApplet *applet);
    void createRootObject(DApplet *applet);
    bool init(DApplet *applet);

    void createChildren(DApplet *applet);
    DApplet *findApplet(DApplet *applet, const QString &pluginId) const;
    DApplet *parentApplet(const QString &pluginId) const;
    void fail(const QString &pluginId, const QString &reason);

    void loadTranslation(const DPluginMetaData &pluginData);
    void removeTranslation(const QString &pluginId);
    void removeTranslations(const QString &pluginId);

    QPointer<DApplet> m_applet = nullptr;
    QPointer<AppletConfigManager> m_configManager = nullptr;
    QString m_pluginId;
    QMap<QString, QTranslator *> m_pluginTranslators;

    D_DECLARE_PUBLIC(DAppletLoader);
};

DAppletLoader::DAppletLoader(DApplet *applet, AppletConfigManager *configManager, QObject *parent)
    : QObject(parent)
    , DObject(*new DAppletLoaderPrivate(this))
{
    D_D(DAppletLoader);
    d->m_applet = applet;
    d->m_configManager = configManager;
    Q_ASSERT(configManager);
}

DAppletLoader::~DAppletLoader()
{

}

void DAppletLoader::setPluginId(const QString &pluginId)
{
    D_D(DAppletLoader);
    d->m_pluginId = pluginId;
}

void DAppletLoader::exec()
{
    D_D(DAppletLoader);
    auto applet = d->m_applet.data();

    if (!d->m_pluginId.isEmpty()) {
        applet = d->findApplet(applet, d->m_pluginId);
        if (applet) {
            return;
        }
        auto containment = qobject_cast<DContainment *>(d->parentApplet(d->m_pluginId));
        if (!containment) {
            d->fail(d->m_pluginId, QStringLiteral("Parent containment was not found."));
            return;
        }

        applet = containment->createApplet(DAppletData(d->m_pluginId));
        if (!applet) {
            d->fail(d->m_pluginId, QStringLiteral("Failed to create applet in its parent containment."));
            return;
        }
    }

    d->loadTranslation(applet->pluginMetaData());

    if (!d->load(applet))
        return;
    
    d->createRootObject(applet);

    if (!d->init(applet))
        return;
}

void DAppletLoader::remove(const QString &pluginId)
{
    D_D(DAppletLoader);
    auto applet = d->findApplet(d->m_applet, pluginId);
    if (auto containment = applet ? qobject_cast<DContainment *>(applet->parentApplet()) : nullptr) {
        containment->removeApplet(applet);
        d->removeTranslations(pluginId);
    }
}

DApplet *DAppletLoader::applet() const
{
    D_DC(DAppletLoader);
    return d->m_applet;
}

void DAppletLoaderPrivate::doCreateRootObject(DApplet *applet)
{
    if (applet->pluginMetaData().url().isEmpty())
        return;

    DQmlEngine *engine = new DQmlEngine(applet, applet);
    QObject::connect(engine, &DQmlEngine::createFinished, applet, [this, applet, engine]() {
        auto rootObject = engine->rootObject();
        applet->setRootObject(rootObject);
        engine->completeCreate();
        if (!rootObject) {
            D_Q(DAppletLoader);
            qCWarning(dsLoaderLog) << "Create root failed:" << applet->pluginId();
            Q_EMIT q->failed(applet->pluginId());
        } else {
            qCDebug(dsLoaderLog) << "Created rootObject for the applet:" << applet->pluginId();
        }
    });
    qCDebug(dsLoaderLog) << "Begin to create rootObject the applet:" << applet->pluginId();
    QMetaObject::invokeMethod(engine, [engine]() {
        if (!engine->create()) {
            engine->deleteLater();
        }
    }, Qt::QueuedConnection);
}

bool DAppletLoaderPrivate::doLoad(DApplet *applet)
{
    D_Q(DAppletLoader);
    Benchmark benchmark(QString("Load applet %1").arg(applet->pluginId()));
    Q_UNUSED(benchmark);
    if (!applet->load()) {
        qCWarning(dsLoaderLog) << "Plugin load failed:" << applet->pluginId();
        if (auto containment = qobject_cast<DContainment *>(applet->parentApplet())) {
            containment->removeApplet(applet);
        }
        Q_EMIT q->failed(applet->pluginId());
        return false;
    }
    return true;
}

bool DAppletLoaderPrivate::doInit(DApplet *applet)
{
    D_Q(DAppletLoader);
    Benchmark benchmark(QString("Init applet %1").arg(applet->pluginId()));
    Q_UNUSED(benchmark);
    if (!applet->init()) {
        qCWarning(dsLoaderLog) << "Plugin init failed:" << applet->pluginId();
        if (auto containment = qobject_cast<DContainment *>(applet->parentApplet())) {
            containment->removeApplet(applet);
        }
        Q_EMIT q->failed(applet->pluginId());
        return false;
    }
    return true;
}

void DAppletLoaderPrivate::createChildren(DApplet *applet)
{
    if (auto containment = qobject_cast<DContainment *>(applet)) {
        const auto data = applet->appletData();
        auto groups = groupList(applet, data);
        for (const auto &item : std::as_const(groups)) {
            if (!m_configManager->isAppletEnabled(item.pluginId()))
                continue;
            if (findApplet(m_applet, item.pluginId()))
                continue;

            auto child = containment->createApplet(item);
            if (!child) {
                continue;
            }
        }
    }
}

DApplet *DAppletLoaderPrivate::findApplet(DApplet *applet, const QString &pluginId) const
{
    if (!applet)
        return nullptr;
    if (applet->pluginId() == pluginId)
        return applet;

    auto containment = qobject_cast<DContainment *>(applet);
    if (!containment)
        return nullptr;

    const auto children = containment->applets();
    for (const auto &child : children) {
        if (auto result = findApplet(child, pluginId))
            return result;
    }
    return nullptr;
}

DApplet *DAppletLoaderPrivate::parentApplet(const QString &pluginId) const
{
    const auto parentPlugin = DPluginLoader::instance()->parentPlugin(pluginId);
    if (!parentPlugin.isValid())
        return m_applet;

    return findApplet(m_applet, parentPlugin.pluginId());
}

void DAppletLoaderPrivate::fail(const QString &pluginId, const QString &reason)
{
    D_Q(DAppletLoader);
    qCWarning(dsLoaderLog) << reason << pluginId;
    Q_EMIT q->failed(pluginId);
}

bool DAppletLoaderPrivate::load(DApplet *applet)
{
    if (!doLoad(applet)) {
        return false;
    }

    createChildren(applet);

    if (auto containment = qobject_cast<DContainment *>(applet)) {
        auto applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {

            load(child);
        }
    }
    return true;
}

void DAppletLoaderPrivate::createRootObject(DApplet *applet)
{
    doCreateRootObject(applet);
    if (auto containment = qobject_cast<DContainment *>(applet)) {
        auto applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {

            createRootObject(child);
        }
    }
}

bool DAppletLoaderPrivate::init(DApplet *applet)
{
    if (!doInit(applet))
        return false;

    if (auto containment = qobject_cast<DContainment *>(applet)) {
        QList<DApplet *> applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {
            init(child);
        }
    }
    return true;
}

void DAppletLoaderPrivate::loadTranslation(const DPluginMetaData &pluginData)
{
    const QString baseDir = pluginData.pluginDir();
    const QString pluginId = pluginData.pluginId();

    if (!m_configManager->isAppletEnabled(pluginId))
        return;

    auto translator = new QTranslator(qApp);
    const QString pluginTranslationDir(baseDir + "/translations/");
    if (translator->load(QLocale::system(), pluginId, QLatin1String("_"), pluginTranslationDir)) {
        m_pluginTranslators[pluginId] = translator;
        qApp->installTranslator(translator);
        qInfo(dsLoaderLog) << "Loaded translation:" << translator->filePath();
    } else {
        qCWarning(dsLoaderLog) << "Failed to load translation:" << pluginTranslationDir << "plugin id:" << pluginId
                               << "locale:" << QLocale::system().uiLanguages();
        translator->deleteLater();
    }

    const auto children = DPluginLoader::instance()->childrenPlugin(pluginId);
    for (const auto &childPluginData : children) {
        loadTranslation(childPluginData);
    }
}

void DAppletLoaderPrivate::removeTranslation(const QString &pluginId)
{
    if (m_pluginTranslators.contains(pluginId)) {
        qApp->removeTranslator(m_pluginTranslators.value(pluginId));
        m_pluginTranslators.value(pluginId)->deleteLater();
        m_pluginTranslators.remove(pluginId);
    }
}

void DAppletLoaderPrivate::removeTranslations(const QString &pluginId)
{
    const auto children = DPluginLoader::instance()->childrenPlugin(pluginId);
    for (const auto &child : children)
        removeTranslations(child.pluginId());
    removeTranslation(pluginId);
}

DS_END_NAMESPACE
