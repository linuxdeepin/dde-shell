// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroupmanager.h"
#include "appgroup.h"
#include "itemspage.h"
#include "appitemmodel.h"
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>

#define TOPLEVEL_FOLDERID 0

namespace apps {

AppGroupManager::AppGroupManager(QAbstractItemModel *referenceModel, QObject *parent)
    : AppGroupManager(referenceModel, Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dde-apps"), parent)
{
    m_config->setParent(this);
}

AppGroupManager::AppGroupManager(QAbstractItemModel *referenceModel, Dtk::Core::DConfig *config, QObject *parent)
    : QStandardItemModel(parent)
    , m_referenceModel(referenceModel)
    , m_config(config)
    , m_referenceSyncTimer(this)
{
    launchpadArrangementConfigMigration();
    loadAppGroupInfo();

    // Source adapters can still be between begin/endRemoveRows while the
    // source emits rowsRemoved. Mutate folders only after those transactions
    // finish, and coalesce application metadata changes into one refresh.
    m_referenceSyncTimer.setSingleShot(true);
    m_referenceSyncTimer.setInterval(0);
    connect(&m_referenceSyncTimer, &QTimer::timeout, this, &AppGroupManager::onReferenceModelChanged);
    if (!m_referenceModel) return;
    connect(m_referenceModel, &QAbstractItemModel::rowsInserted, this, &AppGroupManager::scheduleReferenceSync);
    connect(m_referenceModel, &QAbstractItemModel::rowsRemoved, this, &AppGroupManager::scheduleReferenceSync);
    connect(m_referenceModel, &QAbstractItemModel::modelReset, this, &AppGroupManager::scheduleReferenceSync);
    if (m_referenceModel->metaObject()->indexOfSignal("readyChanged(bool)") >= 0)
        connect(m_referenceModel, SIGNAL(readyChanged(bool)), this, SLOT(scheduleReferenceSync()));
    connect(m_referenceModel, &QAbstractItemModel::dataChanged, this, &AppGroupManager::scheduleReferenceSync);
    onReferenceModelChanged();
}

// Find the item's location. If folderId is -1, search all folders.
ItemPosition AppGroupManager::findItem(const QString &appId, int folderId)
{
    int page, idx;

    for (int i = 0; i < rowCount(); i++) {
        auto group = static_cast<AppGroup*>(itemFromIndex(index(i, 0)));
        if (folderId >= 0 && group->folderId() != folderId) {
            continue;
        }
        std::tie(page, idx) = group->itemsPage()->findItem(appId);
        if (page != -1) {
            return ItemPosition(group->folderId(), page, idx);
        }
    }

    return ItemPosition();
}

void AppGroupManager::appendItemToGroup(const QString &appId, int groupId)
{
    auto folder = group(groupId);
    if (!folder) return;
    folder->itemsPage()->appendItem(appId);
}

bool AppGroupManager::removeItemFromGroup(const QString &appId, int groupId)
{
    auto folder = group(groupId);
    return folder && folder->itemsPage()->removeItem(appId);
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

ItemsPage * AppGroupManager::groupPages(int groupId)
{
    AppGroup * folder = group(groupId);
    if (!folder) return nullptr;
    return folder->itemsPage();
}

void AppGroupManager::bringToFront(const QString & id)
{
    const ItemPosition origPos = findItem(id.endsWith(".desktop") ? id.chopped(8) : id);

    // can only bring top-level item to front

    if (origPos.group() != TOPLEVEL_FOLDERID) {
        return;
    }

    // already at front
    if (origPos.page() == 0 && origPos.pos() == 0) {
        return;
    }

    auto * topLevel = groupPages(0);
    topLevel->moveItemPosition(origPos.page(), origPos.pos(), 0, 0, false);

    saveAppGroupInfo();

    notifyArrangementChanged();
}

void AppGroupManager::commitRearrangeOperation(const QString & dragId, const QString & dropId, DndOperation operation, int pageHint)
{
    if (dragId == dropId) return;

    const ItemPosition dragOrigPos = findItem(dragId);
    const ItemPosition dropOrigPos = findItem(dropId);


    if (dragOrigPos.group() == -1) {
        qWarning() << "Cannot found" << dragId << "in current item arrangement.";
        return;
    }

    if (operation < DndPrepend || operation > DndAppend) return;
    if (dropId != AppGroup::groupIdFromNumber(0) && dropOrigPos.group() < 0) return;
    if (operation != DndJoin && dropOrigPos.group() < 0) return;
    // A folder may only be reordered at top level.
    if (operation != DndJoin && AppGroup::idIsFolder(dragId) && dropOrigPos.group() != 0) return;

    if (operation != DndOperation::DndJoin) {
        // move to dropId's front or back
        if (dragOrigPos.group() == dropOrigPos.group()) {
            // same folder item re-arrangement
            ItemsPage * folder = groupPages(dropOrigPos.group());
            int destination = dropOrigPos.pos() + (operation == DndAppend ? 1 : 0);
            if (dragOrigPos.page() == dropOrigPos.page() && dragOrigPos.pos() < destination) --destination;
            folder->moveItemPosition(dragOrigPos.page(), dragOrigPos.pos(), dropOrigPos.page(), destination, false);
        } else {
            // different folder item arrangement
            ItemsPage * srcFolder = groupPages(dragOrigPos.group());
            ItemsPage * dstFolder = groupPages(dropOrigPos.group());
            srcFolder->removeItem(dragId);
            const bool replaceSourceFolder = srcFolder->itemCount() == 0
                && dropId == AppGroup::groupIdFromNumber(dragOrigPos.group());
            if (replaceSourceFolder) {
                // The last child takes its folder's slot for either edge. Insert
                // before deleting the folder so its position and page stay valid.
                dstFolder->insertItem(dragId, dropOrigPos.page(), dropOrigPos.pos());
            }
            if (srcFolder->pageCount() == 0 && srcFolder != dstFolder && dragOrigPos.group() != 0) {
                removeGroup(dragOrigPos.group());
            }
            if (!replaceSourceFolder) {
                const auto target = findItem(dropId);
                dstFolder->insertItem(dragId, target.page(), target.pos() + (operation == DndAppend ? 1 : 0));
            }
        }
    } else {
        if (AppGroup::idIsFolder(dragId) && dropId != "internal/folders/0") return; // cannot drag folder onto something
        if (dropOrigPos.group() != TOPLEVEL_FOLDERID && dropId != "internal/folders/0") return; // folder inside folder is not allowed

        // the source item will be inside a new folder anyway.
        ItemsPage * srcFolder = groupPages(dragOrigPos.group());
    
        if (AppGroup::idIsFolder(dropId)) {
            // drop into existing folder
            ItemsPage * dstFolder = groupPages(AppGroup::parseGroupId(dropId));
            const int fromPage = dragOrigPos.page();
            if (!dstFolder || pageHint < -1 || pageHint > dstFolder->pageCount()) return;
            const int toPage = pageHint;
            if (srcFolder == dstFolder) {
                if (srcFolder->pageCount() == 1 ||
                    (fromPage == toPage && srcFolder->itemCount(fromPage) == 1)) { // dnd the only item to the same page
                    return;
                }
            }

            // hold the empty page avoid access out of page range!
            srcFolder->removeItem(dragId, false);
            dstFolder->insertItemToPage(dragId, toPage);
            srcFolder->removeEmptyPages();
            if (srcFolder != dstFolder && srcFolder->itemCount() == 0 && dragOrigPos.group() != 0)
                removeGroup(dragOrigPos.group());
        } else {
            srcFolder->removeItem(dragId);
            // we need the app info for the new folder name...
            const auto app = m_referenceModel
                ? m_referenceModel->match(m_referenceModel->index(0, 0), AppItemModel::DesktopIdRole, dropId, 1, Qt::MatchExactly).value(0)
                : QModelIndex();
            const int dropCategories = app.isValid() ? app.data(AppItemModel::DDECategoryRole).toInt() : AppItemModel::Others;
            // make a new folder, move two items into the folder
            QString dstFolderId = assignGroupId();
            appendGroup(dstFolderId, "internal/category/" + QString::number(dropCategories), {{dropId, dragId}});
            if (srcFolder->pageCount() == 0 && dragOrigPos.group() != 0) {
                removeGroup(dragOrigPos.group());
            }
            auto * topLevel = groupPages(0);
            const auto target = findItem(dropId);
            topLevel->insertItem(dstFolderId, target.page(), target.pos());
            topLevel->removeItem(dropId);
        }
    }

    saveAppGroupInfo();

    notifyArrangementChanged();
}

QVariantList AppGroupManager::fromListOfStringList(const QList<QStringList> & list)
{
    QVariantList data;
    for (const auto &items : list) data.append(QVariant::fromValue(items));

    return data;
}

// On AM model changed, add newly installed apps to group (if any) and remove apps that are no longer exists.
void AppGroupManager::onReferenceModelChanged()
{
    if (!m_referenceModel) {
        qWarning() << "referenceModel has been destroyed, skip updating";
        return;
    }

    // Avoid remove all existing records when first time (AM model is not ready).
    if (m_referenceModel->property("ready").isValid() && !m_referenceModel->property("ready").toBool()) {
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
        ItemPosition itemPos = findItem(desktopId);
        if (itemPos.group() == -1) {
            appendItemToGroup(desktopId, TOPLEVEL_FOLDERID);
        }
    }

    // Top level also contains folder IDs, not just desktop IDs.
    for (int i = 0; i < rowCount(); ++i) {
        auto folder = group(index(i, 0));
        if (folder->folderId() > 0) appSet.insert(folder->appId());
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

    saveAppGroupInfo();
    notifyArrangementChanged();
}

void AppGroupManager::launchpadArrangementConfigMigration()
{
    const auto saved = m_config->value("Groups").toList();
    // The old applet used row numbers as folder IDs. Its separate migration
    // flag cannot certify these data; keep the version with the root record.
    for (const auto &value : saved) {
        const auto record = value.toMap();
        if (AppGroup::parseGroupId(record.value("groupId").toString()) == TOPLEVEL_FOLDERID
            && record.value("arrangementVersion").toInt() == 1) return;
    }
    const auto importGroups = [this](QVariantList groups) {
        bool hasRoot = false;
        for (auto &value : groups) {
            auto record = value.toMap();
            if (AppGroup::parseGroupId(record.value("groupId").toString()) != TOPLEVEL_FOLDERID) continue;
            record.insert("arrangementVersion", 1);
            value = record;
            hasRoot = true;
        }
        if (!hasRoot) groups.append(QVariantMap{{"groupId", AppGroup::groupIdFromNumber(TOPLEVEL_FOLDERID)},
                                               {"name", "Top Level"}, {"arrangementVersion", 1}});
        m_config->setValue("Groups", groups);
        m_config->setValue("ArrangementMigrated", true);
    };
    const QString arrangementSettingBasePath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    const QString arrangementSettingPath(QDir(arrangementSettingBasePath).absoluteFilePath("deepin/dde-launchpad/item-arrangement.ini"));
    if (!QFile::exists(arrangementSettingPath)) {
        importGroups(saved);
        return;
    }
    QSettings itemArrangementSettings(arrangementSettingPath, QSettings::NativeFormat);

    itemArrangementSettings.beginGroup("fullscreen");
    const QStringList folderGroups(itemArrangementSettings.childGroups());
    if (!folderGroups.contains("toplevel")) {
        // A partial INI must not replace a complete saved layout.
        importGroups(saved);
        return;
    }

    QVariantList list;

    for (const QString & groupName : folderGroups) {
        itemArrangementSettings.beginGroup(groupName);
        QString folderName = itemArrangementSettings.value("name", QString()).toString();
        int pageCount = itemArrangementSettings.value("pageCount", 0).toInt();

        QVariantMap valueMap;
        valueMap.insert("name", folderName);
        const int groupId = groupName == "toplevel" ? 0 : AppGroup::parseGroupId(groupName);
        if (groupId < 0) {
            itemArrangementSettings.endGroup();
            continue;
        }
        valueMap.insert("groupId", groupId);

        QList<QStringList> pages;
        for (int i = 0; i < pageCount; i++) {
            QStringList items = itemArrangementSettings.value(QString::asprintf("pageItems/%d", i)).toStringList();
            // check if items have ".desktop" suffix, and remove the suffix
            std::transform(items.begin(), items.end(), items.begin(), [](QString &item) {
                if (item.endsWith(".desktop")) {
                    item.chop(8);
                }
                return item;
            });
            pages.append(items);
        }

        valueMap.insert("appItems", fromListOfStringList(pages));
        list.append(valueMap);

        itemArrangementSettings.endGroup();
    }

    // Keep the original INI untouched; subsequent starts use Groups.
    importGroups(list);
}

void AppGroupManager::loadAppGroupInfo()
{
    const auto saved = m_config->value("Groups").toList();
    QHash<int, QVariantMap> groups;
    QList<int> order;
    for (const auto &value : saved) {
        const auto record = value.toMap();
        const int id = AppGroup::parseGroupId(record.value("groupId").toString());
        if (id < 0 || groups.contains(id)) continue;
        groups.insert(id, record);
        if (id != 0) order.append(id);
    }
    // Children take precedence over stale top-level copies from older stores.
    // Each application/folder must occur once, and folders cannot be nested.
    order.append(0);
    QSet<QString> placed;
    for (const int groupId : order) {
        const auto record = groups.value(groupId);
        QList<QStringList> pages;
        for (const auto &value : record.value("appItems").toList()) {
            QStringList page;
            for (auto id : value.toStringList()) {
                if (id.startsWith("internal/")) {
                    const int folderId = AppGroup::parseGroupId(id);
                    if (groupId != 0 || folderId <= 0 || !groups.contains(folderId)) continue;
                    id = AppGroup::groupIdFromNumber(folderId);
                } else if (id.endsWith(".desktop")) {
                    id.chop(8);
                }
                if (id.isEmpty() || placed.contains(id)) continue;
                placed.insert(id);
                page.append(id);
            }
            if (!page.isEmpty()) pages.append(page);
        }
        appendGroup(groupId, record.value("name", groupId == 0 ? "Top Level" : "").toString(), pages);
    }
    // Recover folders saved without a top-level reference by the old manager.
    for (const int groupId : order) {
        const auto id = AppGroup::groupIdFromNumber(groupId);
        if (groupId > 0 && !placed.contains(id)) appendItemToGroup(id, 0);
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
        if (folder->folderId() == TOPLEVEL_FOLDERID) valueMap.insert("arrangementVersion", 1);
        valueMap.insert("appItems", fromListOfStringList(folder->pages()));
        list << valueMap;
    }

    m_config->setValue("Groups", list);
}

QString AppGroupManager::assignGroupId() const
{
    QSet<int> ids;
    for (int i = 0; i < rowCount(); ++i)
        ids.insert(static_cast<AppGroup *>(item(i))->folderId());
    int number = 1;
    while (ids.contains(number)) ++number;
    return AppGroup::groupIdFromNumber(number);
}

AppGroup * AppGroupManager::appendGroup(int groupId, QString groupName, const QList<QStringList> &appItemIDs)
{
    Q_ASSERT(!groupIndexById(groupId).isValid());
    return appendGroup(AppGroup::groupIdFromNumber(groupId), groupName, appItemIDs);
}

AppGroup * AppGroupManager::appendGroup(QString groupId, QString groupName, const QList<QStringList> &appItemIDs)
{
    auto p = new AppGroup(groupId, groupName, appItemIDs);
    appendRow(p);
    return p;
}

void AppGroupManager::removeGroup(int groupId)
{
    if (groupId <= 0) return;
    auto idx = groupIndexById(groupId);
    if (!idx.isValid()) return;
    groupPages(0)->removeItem(AppGroup::groupIdFromNumber(groupId));
    removeRow(idx.row());
}

QHash<int, QByteArray> AppGroupManager::roleNames() const
{
    return {{GroupIdRole, "groupId"}, {AppItemModel::DesktopIdRole, "desktopId"},
            {AppItemModel::NameRole, "name"}};
}

QVariantMap AppGroupManager::itemPosition(const QString &id)
{
    const auto position = findItem(id.endsWith(".desktop") ? id.chopped(8) : id);
    return {{"group", position.group()}, {"page", position.page()}, {"pos", position.pos()}};
}

QVariantMap AppGroupManager::folderInfo(const QString &id)
{
    auto folder = group(AppGroup::parseGroupId(id));
    if (!folder) return {};
    QVariantList contents;
    for (const auto &appId : folder->itemsPage()->allArrangedItems()) {
        if (!m_referenceModel) break; // Missing application metadata is not folder deletion.
        const auto app = m_referenceModel->match(m_referenceModel->index(0, 0), AppItemModel::DesktopIdRole, appId, 1, Qt::MatchExactly).value(0);
        if (!app.isValid()) continue;
        contents.append(QVariantMap{{"id", appId}, {"name", app.data(AppItemModel::NameRole)},
                                   {"iconName", app.data(AppItemModel::IconNameRole)}});
    }
    return {{"id", folder->appId()}, {"name", folder->itemsPage()->name()},
            {"contents", contents}, {"pageCount", folder->itemsPage()->pageCount()}};
}

int AppGroupManager::pageCount(int folderId)
{
    const auto pages = groupPages(folderId);
    return pages ? pages->pageCount() : 0;
}

void AppGroupManager::updateFolderName(int folderId, const QString &name)
{
    auto folder = group(folderId);
    if (!folder || folderId == 0) return;
    folder->itemsPage()->setName(name);
    folder->setData(name, AppItemModel::NameRole);
    saveAppGroupInfo();
    notifyArrangementChanged();
}

int AppGroupManager::createEmptyPage(int folderId)
{
    auto pages = groupPages(folderId);
    if (!pages) return -1;
    pages->appendEmptyPage();
    notifyArrangementChanged();
    return pages->pageCount() - 1;
}

void AppGroupManager::removeEmptyPages()
{
    for (int i = 0; i < rowCount(); ++i) group(index(i, 0))->itemsPage()->removeEmptyPages();
    saveAppGroupInfo();
    notifyArrangementChanged();
}

void AppGroupManager::rearrange(const QString &dragId, const QString &dropId, int operation, int pageHint)
{
    const auto normalize = [](const QString &id) { return id.endsWith(".desktop") ? id.chopped(8) : id; };
    commitRearrangeOperation(normalize(dragId), normalize(dropId), static_cast<DndOperation>(operation), pageHint);
}

void AppGroupManager::notifyArrangementChanged()
{
    if (rowCount()) emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
    emit arrangementChanged();
}

}
