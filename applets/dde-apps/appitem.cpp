// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "appgroupmanager.h"
#include "abstractdesktopinfo.h"
#include "categoryutils.h"
#include "appslaunchtimes.h"
#include "appsdockedhelper.h"

#include <QObject>
#include <DUtil>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace apps {
AppItem::AppItem(AbstractDesktopInfo* desktopInfo, QObject* parent)
    : QObject(parent)
    , m_desktopInfo(desktopInfo)
{
    connect(m_desktopInfo, &AbstractDesktopInfo::nameChanged, this, &AppItem::nameChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::iconNameChanged, this, &AppItem::iconNameChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::nodisplayChanged, this, &AppItem::nodisplayChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::categoriesChanged, this, &AppItem::ddeCategoriesChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::actionsChanged, this, &AppItem::actionsChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::lastLaunchedTimeChanged, this, &AppItem::lastLaunchedTimeChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::installedTimeChanged, this, &AppItem::installedTimeChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::autoStartChanged, this, &AppItem::autoStartChanged, Qt::UniqueConnection);
    connect(m_desktopInfo, &AbstractDesktopInfo::onDesktopChanged, this, &AppItem::onDesktopChanged, Qt::UniqueConnection);
}

AppItem::~AppItem()
{
    m_desktopInfo->deleteLater();
}

void AppItem::launch(const QString& action , const QStringList &fields, const QVariantMap &options)
{
    m_desktopInfo->launch(action, fields, options);
    auto count = AppsLaunchTimesHelper::instance()->getLauncheTimesFor(desktopId());
    AppsLaunchTimesHelper::instance()->setLaunchTimesFor(desktopId(), count + 1);
}

QString AppItem::desktopId() const
{
    return m_desktopInfo->desktopId();
}

QString AppItem::name() const
{
    if (m_desktopInfo->deepinVendor() == "deepin" &&
            !m_desktopInfo->genericName().isEmpty()) {
        return m_desktopInfo->genericName();
    }

    return m_desktopInfo->name();
}

QString AppItem::iconName() const
{
    // TODO: calendar and trash support
    return m_desktopInfo->iconName();
}

QString AppItem::startupWMClass() const
{
    return m_desktopInfo->startupWMClass();
}

bool AppItem::nodisplay() const
{
    return m_desktopInfo->nodisplay();
}

AppItem::DDECategories AppItem::ddeCategories() const
{
    return AppItem::DDECategories(CategoryUtils::parseBestMatchedCategory(m_desktopInfo->categories()));
}

QString AppItem::actions() const
{
    QJsonArray array;
    auto actions = m_desktopInfo->actions();
    for (auto action : actions) {
        QJsonObject menu;
        menu["id"] = action.first;
        menu["name"] = action.second;
        array.append(menu);
    }

    return QJsonDocument(array).toJson();
}

quint64 AppItem::lastLaunchedTime() const
{
    // AM: manager lastLaunchedTime. it should be like launchtimes
    return m_desktopInfo->lastLaunchedTime();
}

quint64 AppItem::installedTime() const
{
    return m_desktopInfo->installedTime();
}

quint64 AppItem::launchedTimes() const
{
    return AppsLaunchTimesHelper::instance()->getLauncheTimesFor(desktopId());
}

QString AppItem::deepinVendor() const
{
    return m_desktopInfo->deepinVendor();
}

int AppItem::groupId() const
{
    return AppGroupManager::instance()->getAppGroupForAppItemId(desktopId());
}

void AppItem::setGroupId(const uint &groupId, const int &pos)
{
    AppGroupManager::instance()->setGropForAppItemId(desktopId(), groupId, pos);
}

bool AppItem::docked() const
{
    return AppsDockedHelper::instance()->isDocked(desktopId());
}

bool AppItem::autoStart() const
{
    return m_desktopInfo->autoStart();
}

bool AppItem::onDesktop() const
{
    return m_desktopInfo;
}

void AppItem::setOnDesktop(bool onDesktop)
{
    m_desktopInfo->setOnDesktop(onDesktop);
}


void AppItem::setDocked(bool docked)
{
    // TODO
}

void AppItem::setAutoStart(bool autoStart)
{
    m_desktopInfo->setAutoStart(autoStart);
}

}