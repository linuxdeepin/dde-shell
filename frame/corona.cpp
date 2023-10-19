// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "corona.h"
#include "private/corona_p.h"

#include "qmlengine.h"

#include <QLoggingCategory>

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

DS_END_NAMESPACE
