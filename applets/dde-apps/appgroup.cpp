// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroup.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "itemspage.h"

#include <QLoggingCategory>
#include <algorithm>

Q_LOGGING_CATEGORY(appGroupLog, "org.deepin.ds.dde-apps.appgroup")

namespace apps {
AppGroup::AppGroup(const QString &groupId, const QString &name, const QList<QStringList> &appIDs)
    : AppItem(groupId, AppItemModel::FolderItemType)
    , m_itemsPage(new ItemsPage(name, groupId == QStringLiteral("internal/folder/0") ? (4 * 8) : (3 * 4)))
{
    setItemsPerPage(m_itemsPage->maxItemCountPerPage());
    setAppName(m_itemsPage->name());
    // folder id is a part of its groupId: "internal/folder/{folderId}"
    setFolderId(groupId.section('/', -1).toInt());

    for (const QStringList &items : appIDs) {
        m_itemsPage->appendPage(items);
    }
}

AppGroup::~AppGroup()
{
    delete m_itemsPage;
}

int AppGroup::folderId() const
{
    return data(AppGroupManager::GroupIdRole).toInt();
}

QList<QStringList> AppGroup::pages() const
{
    return m_itemsPage->allPagedItems();
}

ItemsPage *AppGroup::itemsPage()
{
    return m_itemsPage;
}

void AppGroup::setItemsPerPage(int number)
{
    return setData(number, AppGroupManager::GroupItemsPerPageRole);
}

void AppGroup::setFolderId(int folderId)
{
    setData(folderId, AppGroupManager::GroupIdRole);
}

}

