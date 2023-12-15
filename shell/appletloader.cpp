// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appletloader.h"
#include "pluginloader.h"
#include "applet.h"
#include "containment.h"
#include "appletdata.h"
#include "qmlengine.h"

#include <dobject_p.h>

#include <QMap>
#include <QLoggingCategory>
#include <QElapsedTimer>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE;

Q_LOGGING_CATEGORY(dsLoaderLog, "dde.shell.loader")

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

    bool doLoad(DApplet *applet);
    void doCreateRootObject(DApplet *applet);
    bool doInit(DApplet *applet);

    void load(DApplet *applet);
    void createRootObject(DApplet *applet);
    void init(DApplet *applet);

    void createChildren(DApplet *applet);

    DApplet *m_applet = nullptr;

    D_DECLARE_PUBLIC(DAppletLoader);
};

DAppletLoader::DAppletLoader(class DApplet *applet, QObject *parent)
    : QObject(parent)
    , DObject(*new DAppletLoaderPrivate(this))
{
    D_D(DAppletLoader);
    d->m_applet = applet;
}

DAppletLoader::~DAppletLoader()
{

}

void DAppletLoader::exec()
{
    D_D(DAppletLoader);

    d->load(d->m_applet);

    d->createRootObject(d->m_applet);

    d->init(d->m_applet);
}

DApplet *DAppletLoader::applet() const
{
    D_DC(DAppletLoader);
    return d->m_applet;
}

void DAppletLoaderPrivate::doCreateRootObject(DApplet *applet)
{
    DQmlEngine *engine = new DQmlEngine(applet, applet);
    QObject::connect(engine, &DQmlEngine::createFinished, applet, [this, applet, engine]() {
        auto rootObject = engine->rootObject();
        engine->completeCreate();
        if (!rootObject) {
            D_Q(DAppletLoader);
            qCWarning(dsLoaderLog) << "Create root failed:" << applet->pluginId();
            Q_EMIT q->failed();
        }
        applet->setRootObject(rootObject);
    });
    if (!engine->create()) {
        engine->deleteLater();
    }
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
        Q_EMIT q->failed();
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
        Q_EMIT q->failed();
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

            auto child = containment->createApplet(item);
            if (!child) {
                continue;
            }
        }
    }
}

void DAppletLoaderPrivate::load(DApplet *applet)
{
    if (!doLoad(applet)) {
        return;
    }

    createChildren(applet);

    if (auto containment = qobject_cast<DContainment *>(applet)) {
        auto applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {

            load(child);
        }
    }
}

void DAppletLoaderPrivate::createRootObject(DApplet *applet)
{
    if (auto containment = qobject_cast<DContainment *>(applet)) {
        auto applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {

            createRootObject(child);
        }
    }

    doCreateRootObject(applet);
}

void DAppletLoaderPrivate::init(DApplet *applet)
{
    if (!doInit(applet))
        return;

    if (auto containment = qobject_cast<DContainment *>(applet)) {
        QList<DApplet *> applets = containment->applets();
        for (const auto &child : std::as_const(applets)) {
            init(child);
        }
    }
}

DS_END_NAMESPACE
