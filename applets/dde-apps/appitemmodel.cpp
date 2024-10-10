// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitemmodel.h"
#include "appitem.h"

namespace apps {
AppItemModel::AppItemModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

AppItemModel::~AppItemModel()
{
    for (auto item : m_items) {
        item->deleteLater();
    }
}

int AppItemModel::rowCount(const QModelIndex& parent) const
{
    return m_items.length();
}

QVariant AppItemModel::data(const QModelIndex &index, int role) const
{
    auto i = index.row();
    if (!index.isValid() || i >= m_items.size())
        return {};
    switch (role) {
    case DesktopIdRole:
        return m_items[i]->desktopId();
    case NameRole:
        return m_items[i]->name();
    case IconNameRole:
        return m_items[i]->iconName();
    case StartUpWMClassRole:
        return m_items[i]->startupWMClass();
    case NoDisplayRole:
        return m_items[i]->nodisplay();
    case DDECategoryRole:
        return m_items[i]->ddeCategories();
    case InstalledTimeRole:
        return m_items[i]->installedTime();
    case LastLaunchedTimeRole:
        return m_items[i]->lastLaunchedTime();
    case LaunchedTimesRole:
        return m_items[i]->launchedTimes();
    case DockedRole:
        return m_items[i]->docked();
    case OnDesktopRole:
        return m_items[i]->onDesktop();
    case AutoStartRole:
        return m_items[i]->autoStart();
    }

    return {};
}

bool AppItemModel::setData(const QModelIndex &index, const QVariant &value, int role )
{
    auto i = index.row();
    if (!index.isValid() || i >= m_items.size()) return false;

    switch (role) {
    case OnDesktopRole:
        m_items[i]->setOnDesktop(value.toBool());
        return true;
    case AutoStartRole:
        m_items[i]->setAutoStart(value.toBool());
        return true;
    }

    return false;
}

QModelIndex AppItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (row >= m_items.length() || row < 0) {
        return QModelIndex();
    }

    auto appitem = m_items[row];
    // adata used for QModelIndex internalPointer.
    return createIndex(row, column, appitem);
}

QHash<int, QByteArray> AppItemModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = {
        {AppItemModel::DesktopIdRole, QByteArrayLiteral("desktopId")},
        {AppItemModel::NameRole, QByteArrayLiteral("name")},
        {AppItemModel::IconNameRole, QByteArrayLiteral("iconName")},
        {AppItemModel::StartUpWMClassRole, QByteArrayLiteral("startupWMClass")},
        {AppItemModel::NoDisplayRole, QByteArrayLiteral("noDisplay")},
        {AppItemModel::ActionsRole, QByteArrayLiteral("actions")},
        {AppItemModel::DDECategoryRole, QByteArrayLiteral("ddeCategory")},
        {AppItemModel::InstalledTimeRole, QByteArrayLiteral("installedTime")},
        {AppItemModel::LastLaunchedTimeRole, QByteArrayLiteral("lastLaunchedTime")},
        {AppItemModel::LaunchedTimesRole, QByteArrayLiteral("launchedTimes")},
        {AppItemModel::DockedRole, QByteArrayLiteral("docked")},
        {AppItemModel::OnDesktopRole, QByteArrayLiteral("onDesktop")},
        {AppItemModel::AutoStartRole, QByteArrayLiteral("autoStart")}
    };

    return roleNames;
}

void AppItemModel::addAppItems(const QList<AppItem*> &items)
{
    auto parent = index(rowCount());
    beginInsertRows(parent, rowCount(), rowCount() + items.length() - 1);
    m_items.append(items);
    endInsertRows();

    // connect dataChange
    for (auto item : items) {
        QObject::connect(item, &AppItem::nameChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::NameRole});
        });
        QObject::connect(item, &AppItem::iconNameChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::IconNameRole});
        });
        QObject::connect(item, &AppItem::startupWMClassChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::StartUpWMClassRole});
        });
        QObject::connect(item, &AppItem::nodisplayChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::NoDisplayRole});
        });
        QObject::connect(item, &AppItem::actionsChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::ActionsRole});
        });
        QObject::connect(item, &AppItem::ddeCategoriesChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::DDECategoryRole});
        });
        QObject::connect(item, &AppItem::installedTimeChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::InstalledTimeRole});
        });
        QObject::connect(item, &AppItem::lastLaunchedTimeChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::LastLaunchedTimeRole});
        });
        QObject::connect(item, &AppItem::dockedChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::DockedRole});
        });
        QObject::connect(item, &AppItem::onDesktopChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::OnDesktopRole});
        });
        QObject::connect(item, &AppItem::autoStartChanged, this, [this, item](){
            auto modelIndex = index(m_items.indexOf(item));
            if (!modelIndex.isValid())
                return;
            Q_EMIT dataChanged(modelIndex, modelIndex, {AppItemModel::AutoStartRole});
        });
    }
}

void AppItemModel::removeAppItems(const QList<AppItem*> &items)
{
    for (auto item : items) {
        auto pos = m_items.indexOf(item);
        if (-1 == pos)
            continue;

        beginRemoveRows(QModelIndex(), pos, pos);
        m_items.remove(pos);
        item->deleteLater();
        endRemoveRows();
    }
}
}