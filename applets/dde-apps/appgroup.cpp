// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroup.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "itemspage.h"

#include <QLoggingCategory>
#include <algorithm>

Q_LOGGING_CATEGORY(appGroupLog, "org.deepin.dde.shell.dde-apps.appgroup")

namespace apps {
AppGroup::AppGroup(const QString &groupId, const QString &name, const QList<QStringList> &appIDs)
    : AppItem(groupId, AppItemModel::FolderItemType)
    , m_itemsPage(new ItemsPage(name, parseGroupId(groupId) == 0 ? (4 * 8) : (3 * 4)))
{
    setItemsPerPage(m_itemsPage->maxItemCountPerPage());
    setAppName(m_itemsPage->name());
    // folder id is a part of its groupId: "internal/folders/{folderId}"
    setFolderId(parseGroupId(groupId));

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

bool AppGroup::idIsFolder(const QString & id)
{
    return parseGroupId(id) >= 0;
}

QString AppGroup::groupIdFromNumber(int groupId)
{
    return QStringLiteral("internal/folders/%1").arg(groupId);
}

int AppGroup::parseGroupId(const QString & id)
{
    constexpr QLatin1StringView prefix("internal/folders/");
    if (!id.startsWith(prefix))
        return -1;

    const QStringView number = QStringView(id).sliced(prefix.size());

    if (number.isEmpty())
        return -1;

    for (const QChar ch : number) {
        if (!ch.isDigit())
            return -1;
    }

    bool ok = false;
    const int groupId = number.toInt(&ok);
    return ok && groupId >= 0 ? groupId : -1;
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

