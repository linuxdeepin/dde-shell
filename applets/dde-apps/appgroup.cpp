// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
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
    : AppItem(normalizeGroupId(groupId), AppItemModel::FolderItemType)
    , m_itemsPage(new ItemsPage(name, parseGroupId(groupId) == 0 ? (4 * 8) : (3 * 4)))
{
    setItemsPerPage(m_itemsPage->maxItemCountPerPage());
    setAppName(m_itemsPage->name());
    QObject::connect(m_itemsPage, &ItemsPage::nameChanged, m_itemsPage, [this]() {
        setAppName(m_itemsPage->name());
    });
    // folder id is the numeric suffix of the normalized launcher group id.
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
    bool isNumericId = false;
    id.toInt(&isNumericId);

    return isNumericId ||
           id.startsWith(QStringLiteral("internal/folder/")) ||
           id.startsWith(QStringLiteral("internal/folders/")) ||
           id.startsWith(QStringLiteral("internal/group/"));
}

QString AppGroup::normalizeGroupId(const QString &id)
{
    bool isNumericId = false;
    const int numericId = id.toInt(&isNumericId);
    if (isNumericId) {
        return groupIdFromNumber(numericId);
    }

    if (!idIsFolder(id)) {
        return id;
    }

    return QStringLiteral("internal/folders/%1").arg(parseGroupId(id));
}

QString AppGroup::groupIdFromNumber(int groupId)
{
    return QStringLiteral("internal/folders/%1").arg(groupId);
}

int AppGroup::parseGroupId(const QString & id)
{
    return id.section(QLatin1Char('/'), -1).toInt();
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
