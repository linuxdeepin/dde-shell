// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "containment.h"
#include "private/containment_p.h"

#include "pluginloader.h"
#include "pluginmetadata.h"
#include "appletitem.h"

#include <QLoggingCategory>

DS_BEGIN_NAMESPACE
DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DContainment::DContainment(QObject *parent)
    : DContainment(*new DContainmentPrivate(this), parent)
{
}

DContainment::DContainment(DContainmentPrivate &dd, QObject *parent)
    : DApplet(dd, parent)
{

}

DContainment::~DContainment()
{

}

DApplet *DContainment::createApplet(const QString &pluginId)
{
    D_D(DContainment);
    const auto children = DPluginLoader::instance()->childrenPlugin(this->pluginId());
    if (!children.contains(DPluginLoader::instance()->plugin(pluginId))) {
        return nullptr;
    }
    auto applet = DPluginLoader::instance()->loadApplet(pluginId);
    if (applet) {
        applet->setParent(this);
        d->m_applets.append(applet);
    }
    return applet;
}

void DContainment::removeApplet(DApplet *applet)
{
    D_D(DContainment);
    if (d->m_applets.contains(applet)) {
        d->m_applets.removeOne(applet);
        applet->deleteLater();
    }
}

QList<DApplet *> DContainment::applets() const
{
    D_DC(DContainment);
    return d->m_applets;
}

QList<QObject *> DContainment::appletItems()
{
    D_D(DContainment);

    return d->m_appletItems;
}

bool DContainment::load()
{
    const auto children = DPluginLoader::instance()->childrenPlugin(pluginId());
    for (const auto &item : children) {
        const QString id = item.pluginId();
        auto applet = createApplet(id);

        if (!applet)
            continue;

        if (!applet->load()) {
            removeApplet(applet);
        }
    }
    return DApplet::load();
}

bool DContainment::init()
{
    D_D(DContainment);

    QList<DApplet *> failedApplets;
    for (auto applet : applets()) {
        auto appletItem = DAppletItem::itemForApplet(applet);
        if (appletItem && !d->m_appletItems.contains(appletItem)) {
            d->m_appletItems << appletItem;
        }

        if (!applet->init()) {
            failedApplets << applet;
            continue;
        }
    }
    bool res = DApplet::init();

    for (auto applet: failedApplets) {
        removeApplet(applet);
    }

    Q_EMIT appletItemsChanged();
    return res;
}

DS_END_NAMESPACE
