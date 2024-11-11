// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroupmanager.h"
#include "appgroup.h"
#include "amappitemmodel.h"

#include <QtConcurrent>

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

    connect(m_dumpTimer, &QTimer::timeout, this, [this](){
        dumpAppGroupInfo();
    });
    connect(this, &AppGroupManager::dataChanged, this, &AppGroupManager::dumpAppGroupInfo);
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

std::tuple<int, int, int> AppGroupManager::getAppGroupInfo(const QString &appId)
{
    std::tuple<int, int, int> res = {-1, -1, -1};
    do {
        int groupPos, pagePos;
        std::tie(groupPos, pagePos) = m_map.value(appId, std::make_tuple<int, int>(-1, -1));
        auto groupIndex = index(groupPos, 0);

        if (!groupIndex.isValid())
            break;

        auto pages = groupIndex.data(GroupAppItemsRole).toList();
        if (pages.length() == 0)
            break;

        auto items = pages.value(pagePos).toStringList();
        if (items.length() == 0)
            break;

        auto itemPos = items.indexOf(appId);
        res = std::make_tuple(groupPos, pagePos, itemPos);
    } while (0);

    return res;
}

void AppGroupManager::setAppGroupInfo(const QString &appId, std::tuple<int, int, int> groupInfo)
{
    int groupPos, pagePos, itemPos;
    std::tie(groupPos, pagePos, itemPos) = groupInfo;

    auto groupIndex = index(groupPos, 0);
    if (!groupIndex.isValid()) {
        return;
    }

    auto appItems = groupIndex.data(GroupAppItemsRole).value<QList<QStringList>>();
    int groupItemsPerPage = groupIndex.data(GroupItemsPerPageRole).toInt();

    for (int i = pagePos; i < appItems.length() - 1; i++) {
        appItems[i].insert(itemPos, appId);
        m_map.insert(appId, std::make_tuple(groupPos, pagePos));

        // 本页最后一位元素插入到下页
        if (appItems[i].length() > groupItemsPerPage) {
            auto item = appItems[i].takeLast();

            appItems[i + 1].insert(0, item);
            m_map.insert(item, std::make_tuple(groupPos, i + 1));
        }
    }

    if (appItems.length() > 1 && appItems.last().length() > groupItemsPerPage) {
        auto item = appItems.last().takeLast();
        appItems.append({item});
    }

    if (pagePos > appItems.length()) {
        appItems.append({appId});
    }

    QVariantList data;
    std::transform(appItems.begin(), appItems.end(), std::back_inserter(data), [](const QStringList &items) {
        QVariantList tmp;
        std::transform(items.begin(), items.end(), std::back_inserter(tmp), [](const auto &item) {
            return QVariant::fromValue(item);
        });
        return tmp;
    });

    setData(groupIndex, data);
}

void AppGroupManager::loadAppGroupInfo()
{
    auto groups = m_config->value("Groups").toList();
    for (int i = 0; i < groups.length(); i++) {
        auto group = groups[i].toMap();
        auto folderId = group.value("folderId", "").toString();
        auto name = group.value("name", "").toString();
        auto pages = group.value("appItems", QVariantList()).toList();
        QList<QStringList> items;

        for (int j = 0; j < pages.length(); j++) {
            auto page = pages[j].toStringList();
            items << page;
            std::for_each(page.begin(), page.end(), [this, i, j](const QString &item) {
                m_map.insert(item, std::make_tuple(i, j));
            });
        }

        if (folderId.isEmpty()) {
            folderId = assignGroupId();
        }
        auto p = new AppGroup(folderId, name, items);
        appendRow(p);
    }
}

void AppGroupManager::dumpAppGroupInfo()
{
    QVariantList list;
    for (int i = 0; i < rowCount(); i++) {
        auto data = index(i, 0);
        QVariantMap valueMap;
        valueMap.insert("name", data.data(AppItemModel::NameRole));
        valueMap.insert("folderId", data.data(AppItemModel::DesktopIdRole));
        valueMap.insert("appItems", data.data(GroupAppItemsRole));
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
