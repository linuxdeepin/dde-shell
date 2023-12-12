// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "private/panel_p.h"

#include "qmlengine.h"

#include <QLoggingCategory>

#include <QDir>
#include <DIconTheme>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DPanel::DPanel(QObject *parent)
    : DContainment(*new DPanelPrivate(this), parent)
{
}

DPanel::~DPanel()
{

}

QQuickWindow *DPanel::window() const
{
    D_DC(DPanel);
    return qobject_cast<QQuickWindow *>(d->m_rootObject);
}

bool DPanel::load(const DAppletData &data)
{
    D_D(DPanel);
    return DContainment::load(data);
}

bool DPanel::init()
{
    D_D(DPanel);
    d->initDciSearchPaths();

    auto applet = this;

    std::unique_ptr<DQmlEngine> engine(new DQmlEngine(applet, applet));

    auto rootObject = engine->beginCreate();

    auto window = qobject_cast<QQuickWindow *>(rootObject);
    if (window) {
        applet->d_func()->setRootObject(window);
    }

    bool res = DContainment::init();

    if (res) {
        engine->completeCreate();
        d->m_engine = engine.release();
    }

    return res;
}

void DPanelPrivate::initDciSearchPaths()
{
    D_Q(DPanel);
    DGUI_USE_NAMESPACE;
    auto dciPaths = DIconTheme::dciThemeSearchPaths();
    QList<DApplet *> list = m_applets;
    list.append(q);
    for (const auto &item : list) {
        QDir root(item->pluginMetaData().pluginDir());
        if (root.exists("icons")) {
            dciPaths.push_back(root.absoluteFilePath("icons"));
        }
    }
    DIconTheme::setDciThemeSearchPaths(dciPaths);
}

DS_END_NAMESPACE
