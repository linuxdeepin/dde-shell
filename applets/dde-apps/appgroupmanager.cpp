// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroupmanager.h"
#include "appgroup.h"
#include "itemspage.h"
#include "amappitemmodel.h"

#define TOPLEVEL_FOLDERID 0

namespace apps {

AppGroupManager::AppGroupManager(AMAppItemModel * referenceModel, QObject *parent)
    : QStandardItemModel(parent)
    , m_referenceModel(referenceModel)
    , m_config(Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dde-apps", "", this))
    , m_dumpTimer(new QTimer(this))
{
    m_dumpTimer->setSingleShot(true);
    m_dumpTimer->setInterval(1000);

    loadAppGroupInfo();

    connect(m_referenceModel, &AMAppItemModel::rowsInserted, this, [this](){
        onReferenceModelChanged();
        saveAppGroupInfo();
    });
    connect(m_referenceModel, &AMAppItemModel::rowsRemoved, this, [this](){
        onReferenceModelChanged();
        saveAppGroupInfo();
    });
    connect(m_dumpTimer, &QTimer::timeout, this, [this](){
        saveAppGroupInfo();
    });
    connect(this, &AppGroupManager::dataChanged, this, &AppGroupManager::saveAppGroupInfo);
}

QVariant AppGroupManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == GroupIdRole) {
        return index.row();
    }

    return QStandardItemModel::data(index, role);
}

// Find the item's location. If folderId is -1, search all folders.
std::tuple<int, int, int> AppGroupManager::findItem(const QString &appId, int folderId)
{
    int page, idx;

    for (int i = 0; i < rowCount(); i++) {
        auto group = static_cast<AppGroup*>(itemFromIndex(index(i, 0)));
        if (folderId >= 0 && group->folderId() != folderId) {
            continue;
        }
        std::tie(page, idx) = group->itemsPage()->findItem(appId);
        if (page != -1) {
            return std::make_tuple(group->folderId(), page, idx);
        }
    }

    return std::make_tuple(-1, -1, -1);
}

void AppGroupManager::appendItemToGroup(const QString &appId, int groupId)
{
    auto folder = group(groupId);
    Q_CHECK_PTR(folder);
    folder->itemsPage()->appendItem(appId);
}

bool AppGroupManager::removeItemFromGroup(const QString &appId, int groupId)
{
    auto folder = group(groupId);
    Q_CHECK_PTR(folder);
    return folder->itemsPage()->removeItem(appId);
}

QModelIndex AppGroupManager::groupIndexById(int groupId)
{
    for (int i = 0; i < rowCount(); i++) {
        auto groupIndex = index(i, 0);
        auto data = groupIndex.data(GroupIdRole);
        if (data.toInt() == groupId) {
            return groupIndex;
        }
    }
    return QModelIndex();
}

AppGroup * AppGroupManager::group(int groupId)
{
    auto groupIndex = groupIndexById(groupId);
    if (!groupIndex.isValid()) {
        return nullptr;
    }
    auto group = static_cast<AppGroup*>(itemFromIndex(groupIndex));
    return group;
}

AppGroup * AppGroupManager::group(QModelIndex idx)
{
    if (!idx.isValid()) {
        return nullptr;
    }
    auto group = static_cast<AppGroup*>(itemFromIndex(idx));
    return group;
}

QVariantList AppGroupManager::fromListOfStringList(const QList<QStringList> & list)
{
    QVariantList data;
    std::transform(list.begin(), list.end(), std::back_inserter(data), [](const QStringList &items) {
        QVariantList tmp;
        std::transform(items.begin(), items.end(), std::back_inserter(tmp), [](const auto &item) {
            return QVariant::fromValue(item);
        });
        return items;
    });

    return data;
}

// On AM model changed, add newly installed apps to group (if any) and remove apps that are no longer exists.
void AppGroupManager::onReferenceModelChanged()
{
    // Avoid remove all existing records when first time (AM model is not ready).
    if (m_referenceModel->rowCount() == 0) {
        qDebug() << "referenceModel not ready, wait for next time";
        return;
    }

    QSet<QString> appSet;
    for (int i = 0; i < m_referenceModel->rowCount(); i++) {
        const auto modelIndex = m_referenceModel->index(i, 0);
        const bool noDisplay = m_referenceModel->data(modelIndex, AppItemModel::NoDisplayRole).toBool();
        if (noDisplay) {
            continue;
        }
        const QString & desktopId = m_referenceModel->data(m_referenceModel->index(i, 0), AppItemModel::DesktopIdRole).toString();
        appSet.insert(desktopId);
        // add all existing ones if they are not already in
        int folder, page, idx;
        std::tie(folder, std::ignore, std::ignore) = findItem(desktopId);
        if (folder == -1) {
            appendItemToGroup(desktopId, TOPLEVEL_FOLDERID);
        }
    }

    // remove all non-existing ones
    for (int i = rowCount() - 1; i >= 0; i--) {
        auto folder = group(index(i, 0));
        Q_CHECK_PTR(folder);
        folder->itemsPage()->removeItemsNotIn(appSet);
        // check if group itself is also empty, remove them too.
        if (folder->itemsPage()->itemCount() == 0 && folder->folderId() != TOPLEVEL_FOLDERID) {
            QString groupId = folder->appId();
            removeRow(i);
            removeItemFromGroup(groupId, TOPLEVEL_FOLDERID);
        }
    }

    // TODO: emit datachanged signal?
    // TODO: save item arrangement to user data?
}

void AppGroupManager::loadAppGroupInfo()
{
    auto groups = m_config->value("Groups").toList();

    for (int i = 0; i < groups.length(); i++) {
        auto group = groups[i].toMap();
        auto groupId = group.value("groupId", "").toString();
        auto name = group.value("name", "").toString();
        auto pages = group.value("appItems", QVariantList()).toList();
        QList<QStringList> items;

        for (int j = 0; j < pages.length(); j++) {
            auto page = pages[j].toStringList();
            items << page;
        }

        if (groupId.isEmpty()) {
            groupId = assignGroupId();
        }
        auto p = new AppGroup(groupId, name, items);
        appendRow(p);
    }

    // always ensure top-level group exists
    if (rowCount() == 0) {
        auto p = new AppGroup(assignGroupId(), "Top Level", {});
        Q_ASSERT(p->folderId() == TOPLEVEL_FOLDERID);
        appendRow(p);
    }
}

void AppGroupManager::saveAppGroupInfo()
{
    QVariantList list;
    for (int i = 0; i < rowCount(); i++) {
        auto folder = group(index(i, 0));
        QVariantMap valueMap;
        valueMap.insert("name", folder->data(AppItemModel::NameRole));
        valueMap.insert("groupId", folder->appId());
        valueMap.insert("appItems", fromListOfStringList(folder->pages()));
        list << valueMap;
    }

    m_config->setValue("Groups", list);
}

QString AppGroupManager::assignGroupId() const
{
    QStringList knownGroupIds;
    for (int i = 0; i < rowCount(); i++) {
        auto group = index(i, 0);
        knownGroupIds.append(group.data(AppItemModel::DesktopIdRole).toString());
    }

    int idNumber = 0;
    while (knownGroupIds.contains(QString("internal/group/%1").arg(idNumber))) {
        idNumber++;
    }

    return QString("internal/group/%1").arg(idNumber);
}

}
