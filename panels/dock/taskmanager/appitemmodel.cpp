// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "dsglobal.h"
#include "appitemmodel.h"

#include <algorithm>

#include <QVariant>
#include <QAbstractListModel>

DS_BEGIN_NAMESPACE
namespace dock {
AppItemModel* AppItemModel::instance()
{
    static AppItemModel* appItemModel = nullptr;
    if (appItemModel == nullptr) {
        appItemModel = new AppItemModel();
    }
    return appItemModel;
}

AppItemModel::AppItemModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_recentSize(3)
{
}

QHash<int,QByteArray> AppItemModel::roleNames() const
{
    return {{AppItemModel::ItemIdRole, "itemId"},
        {AppItemModel::NameRole, "name"},
        {AppItemModel::IconNameRole, "iconName"},
        {AppItemModel::ActiveRole, "active"},
        {AppItemModel::MenusRole, "menus"},
        {AppItemModel::DockedRole, "docked"},
        {AppItemModel::WindowsRole, "windows"}
    };
}

int AppItemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_appItems.size();
}

QVariant AppItemModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        return QVariant();
    }

    auto item = m_appItems[index.row()];
    switch (role) {
        case AppItemModel::ItemIdRole: return item->id();
        case AppItemModel::NameRole: return item->name();
        case AppItemModel::IconNameRole: return item->icon();
        case AppItemModel::ActiveRole: return item->isActive();
        case AppItemModel::MenusRole: return item->menus();
        case AppItemModel::DockedRole: return item->isDocked();
        case AppItemModel::WindowsRole: return item->windows();
    }
    return QVariant();
}

void AppItemModel::moveTo(const QString &id, int dIndex)
{
    auto item = getAppItemById(id);
    int sIndex = m_appItems.indexOf(item);
    if (sIndex == dIndex) {
        return;
    }
    if (sIndex + 1 == dIndex) {
        // Do not move from sIndex to sIndex + 1, as endMoveRows is not trivial, this operation equals do nothing.
        // FIXME: maybe this is a bug of Qt? but swap these two is a compatible fix
        std::swap(sIndex, dIndex);
    }
    beginMoveRows(QModelIndex(), sIndex, sIndex, QModelIndex(), dIndex);
    m_appItems.move(sIndex, dIndex);
    endMoveRows();
}

QPointer<AppItem> AppItemModel::getAppItemById(const QString& id) const
{
    auto it = std::find_if(m_appItems.begin(), m_appItems.end(),[id](QPointer<AppItem> item){
        return item->id() == id;
    });

    return it == m_appItems.end() ? nullptr : *it;
}

void AppItemModel::addAppItem(QPointer<AppItem> item)
{
    if (m_appItems.contains(item)) return;

    connect(item.get(), &AppItem::destroyed, this, &AppItemModel::onAppItemDestroyed, Qt::UniqueConnection);
    connect(item.get(), &AppItem::nameChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);
    connect(item.get(), &AppItem::iconChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);
    connect(item.get(), &AppItem::activeChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);
    connect(item.get(), &AppItem::menusChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);
    connect(item.get(), &AppItem::dockedChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);
    connect(item.get(), &AppItem::itemWindowCountChanged, this, &AppItemModel::onAppItemChanged, Qt::QueuedConnection);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_appItems.append(item);
    endInsertRows();
    Q_EMIT appItemAdded();
}

void AppItemModel::onAppItemDestroyed()
{
    auto item = qobject_cast<AppItem*>(sender());
    auto beginIndex = m_appItems.indexOf(item);
    auto lastIndex = m_appItems.lastIndexOf(item);

    if (beginIndex == -1 || lastIndex == -1) return;

    beginRemoveRows(QModelIndex(), beginIndex, lastIndex);
    m_appItems.removeAll(item);
    endRemoveRows();
    Q_EMIT appItemRemoved();
}

void AppItemModel::onAppItemChanged()
{
    auto item = qobject_cast<AppItem*>(sender());
    if (!item) return;
    QModelIndexList indexes = match(index(0, 0, QModelIndex()),
                                    AppItemModel::ItemIdRole, item->id(), 1, Qt::MatchExactly);
    Q_EMIT dataChanged(indexes.first(), indexes.last());
}
}
DS_END_NAMESPACE
