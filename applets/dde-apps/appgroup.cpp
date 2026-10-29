// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
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
    return id.startsWith(QStringLiteral("internal/folders/")) && parseGroupId(id) >= 0;
}

QString AppGroup::groupIdFromNumber(int groupId)
{
    return QStringLiteral("internal/folders/%1").arg(groupId);
}

int AppGroup::parseGroupId(const QString & id)
{
    // Accept legacy IDs while loading, always write the canonical plural form.
    const QStringList prefixes{QStringLiteral("internal/folders/"), QStringLiteral("internal/folder/"), QStringLiteral("internal/group/")};
    QString number = id;
    for (const auto &prefix : prefixes) {
        if (id.startsWith(prefix)) {
            number = id.mid(prefix.size());
            break;
        }
    }
    bool ok = false;
    const int result = number.toInt(&ok);
    return ok && result >= 0 ? result : -1;
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

