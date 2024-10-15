// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroup.h"
#include "appgroupmanager.h"

#include <QLoggingCategory>
#include <algorithm>

Q_LOGGING_CATEGORY(appGroupLog, "org.deepin.ds.dde-apps.appgroup")

namespace apps {
AppGroup::AppGroup(const QString &name, const QList<QStringList> &appIDs)
    : QStandardItem()
{
    setName(name);
    setAppItems(appIDs);
}

QString AppGroup::name() const
{
    return data(AppGroupManager::GroupNameRole).toString();
}

void AppGroup::setName(const QString &name)
{
    return setData(name, AppGroupManager::GroupNameRole);
}

QList<QStringList> AppGroup::appItems() const
{
    QList<QStringList> res;
    auto pages = data(AppGroupManager::GroupAppItemsRole).toList();
    std::transform(pages.begin(), pages.end(), std::back_inserter(res), [](const QVariant &data) {
        return data.toStringList();
    });

    return res;
}

void AppGroup::setAppItems(const QList<QStringList> &items)
{
    QVariantList data;
    std::transform(items.begin(), items.end(), std::back_inserter(data), [](const QStringList &c) {
        QVariantList tmp;
        std::transform(c.begin(), c.end(), std::back_inserter(tmp), [](const QString &s) {
            return s;
        });
        return tmp;
    });
    return setData(data, AppGroupManager::GroupAppItemsRole);
}
}

