// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroup.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"

#include <QLoggingCategory>
#include <algorithm>

Q_LOGGING_CATEGORY(appGroupLog, "org.deepin.ds.dde-apps.appgroup")

namespace apps {
AppGroup::AppGroup(const QString &groupId, const QString &name, const QList<QStringList> &appIDs)
    : AppItem(groupId, AppItemModel::FolderItemType)
{
    if (groupId == QStringLiteral("internal/folder/0")) {
        setItemsPerPage(4 * 8);
    } else {
        setItemsPerPage(3 * 4);
    }
    setAppName(name);
    setAppItems(appIDs);
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

void AppGroup::setItemsPerPage(int number)
{
    return setData(number, AppGroupManager::GroupItemsPerPageRole);
}

}

