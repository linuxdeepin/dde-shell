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

DApplet *DContainment::createApplet(const DAppletData &data)
{
    D_D(DContainment);
    const auto children = DPluginLoader::instance()->childrenPlugin(this->pluginId());
    if (!children.contains(DPluginLoader::instance()->plugin(data.pluginId()))) {
        return nullptr;
    }
    auto applet = DPluginLoader::instance()->loadApplet(data.pluginId(), data.id());
    if (!applet) {
        return nullptr;
    }

    applet->setParent(this);

    if (!applet->load(data)) {
        return nullptr;
    }
    d->m_applets.append(applet);
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

DApplet *DContainment::applet(const QString &id) const
{
    D_DC(DContainment);
    for (auto item : d->m_applets) {
        if (item->id() == id)
            return item;
    }
    return nullptr;
}

bool DContainment::load(const DAppletData &data)
{
    D_D(DContainment);

    for (const auto &item : d->groupList(data)) {
        createApplet(item);
    }
    return DApplet::load(data);
}

bool DContainment::init()
{
    D_D(DContainment);

    QList<DApplet *> failedApplets;
    for (const auto &applet : applets()) {
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

    for (const auto &applet: failedApplets) {
        removeApplet(applet);
    }

    Q_EMIT appletItemsChanged();
    return res;
}

QList<DAppletData> DContainmentPrivate::groupList(const DAppletData &data) const
{
    if (!data.groupList().isEmpty())
        return data.groupList();

    QList<DAppletData> groups;
    const auto children = DPluginLoader::instance()->childrenPlugin(m_metaData.pluginId());
    for (const auto &item : children) {
        groups << DAppletData::fromPluginMetaData(item);
    }
    return groups;
}

DS_END_NAMESPACE
