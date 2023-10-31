// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "corona.h"
#include "private/corona_p.h"

#include "qmlengine.h"

#include <QLoggingCategory>

#include <QDir>
#include <DIconTheme>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DCorona::DCorona(QObject *parent)
    : DContainment(*new DCoronaPrivate(this), parent)
{
}

DCorona::~DCorona()
{

}

QQuickWindow *DCorona::window() const
{
    D_DC(DCorona);
    return d->m_window;
}

void DCorona::load()
{
    D_D(DCorona);
    DContainment::load();
}

void DCorona::init()
{
    D_D(DCorona);
    d->initDciSearchPaths();

    auto applet = this;

    DQmlEngine *engine = new DQmlEngine(applet, applet);

    auto rootObject = engine->beginCreate();

    auto window = qobject_cast<QQuickWindow *>(rootObject);
    if (window) {
        d->m_window = window;
        d->m_window->setProperty("_ds_window_applet", QVariant::fromValue(applet));
    }

    DContainment::init();

    engine->completeCreate();
}

void DCoronaPrivate::initDciSearchPaths()
{
    D_Q(DCorona);
    DGUI_USE_NAMESPACE;
    auto dciPaths = DIconTheme::dciThemeSearchPaths();
    QList<DApplet *> list = m_applets;
    list.append(q);
    for (auto item : list) {
        QDir root(item->pluginMetaData().pluginDir());
        if (root.exists("icons")) {
            dciPaths.push_back(root.absoluteFilePath("icons"));
        }
    }
    DIconTheme::setDciThemeSearchPaths(dciPaths);
}

DS_END_NAMESPACE
