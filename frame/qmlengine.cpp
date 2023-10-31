// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qmlengine.h"
#include "applet.h"

#include <dobject_p.h>
#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

class DQmlEnginePrivate : public DObjectPrivate
{
public:
    explicit DQmlEnginePrivate(DQmlEngine *qq)
        : DObjectPrivate(qq)
    {

    }
    DApplet *m_applet = nullptr;
    QQmlContext *m_context = nullptr;
    QQmlComponent *m_component = nullptr;
    QObject *m_rootObject = nullptr;
    QQmlEngine *engine()
    {
        static QQmlEngine *s_engine = nullptr;
        if (!s_engine) {
            s_engine = new QQmlEngine();
            const QString rootDir = QCoreApplication::applicationDirPath();
            s_engine->addImportPath(rootDir + "/../plugins");
            s_engine->addImportPath(DDE_SHELL_QML_INSTALL_DIR);
            qCDebug(dsLog()) << "Engine importPaths" << s_engine->importPathList();
        }
        return s_engine;
    }
    QString appletUrl() const
    {
        if (!m_applet)
            return QString();

        auto url = m_applet->pluginMetaData().value("Url").toString();
        if (url.isEmpty())
            return QString();

        return QDir(m_applet->pluginMetaData().pluginDir()).absoluteFilePath(url);
    }
};

DQmlEngine::DQmlEngine(QObject *parent)
    : DQmlEngine(nullptr, parent)
{

}

DQmlEngine::DQmlEngine(DApplet *applet, QObject *parent)
    : QObject(parent)
    , DObject(*new DQmlEnginePrivate(this))
{
    D_D(DQmlEngine);
    d->m_applet = applet;
}

DQmlEngine::~DQmlEngine()
{
}

QObject *DQmlEngine::beginCreate()
{
    D_D(DQmlEngine);
    QScopedPointer<QQmlComponent> component(new QQmlComponent(engine(), this));
    const QString url = d->appletUrl();
    if (url.isEmpty())
        return nullptr;

    component->loadUrl(url);
    if (component->isError()) {
        qCWarning(dsLog()) << "Loading url failed" << component->errorString();
        return nullptr;
    }
    auto context = new QQmlContext(engine(), d->m_applet);
    auto object = component->beginCreate(context);
    d->m_context = context;
    d->m_rootObject = object;
    d->m_component = component.take();
    return object;
}

void DQmlEngine::completeCreate()
{
    D_D(DQmlEngine);
    if (!d->m_component)
        return;

    if (!d->m_component->isReady())
        return;

    d->m_component->completeCreate();
}

QObject *DQmlEngine::rootObject() const
{
    D_DC(DQmlEngine);
    return d->m_rootObject;
}

QQmlEngine *DQmlEngine::engine()
{
    D_D(DQmlEngine);
    return d->engine();
}

DS_END_NAMESPACE
