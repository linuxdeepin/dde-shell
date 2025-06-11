// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rolegroupmodel.h"

#include <QList>

RoleGroupModel::RoleGroupModel(QAbstractItemModel *sourceModel, int role, QObject *parent)
    : QAbstractProxyModel(parent)
    , m_roleForDeduplication(role)
{
    RoleGroupModel::setSourceModel(sourceModel);
}

void RoleGroupModel::setDeduplicationRole(const int &role)
{
    if (role != m_roleForDeduplication) {
        m_roleForDeduplication = role;
        rebuildTreeSource();
    }
}

int RoleGroupModel::deduplicationRole() const
{
    return m_roleForDeduplication;
}

void RoleGroupModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel()) {
        sourceModel()->disconnect(this);
    }

    QAbstractProxyModel::setSourceModel(model);

    rebuildTreeSource();
    if (sourceModel() == nullptr)
        return;

    connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int first, int last) {
        adjustMap(first, (last - first) + 1);

        for (int i = first; i <= last; i++) {
            auto sourceIndex = sourceModel()->index(i, 0);
            auto data = sourceIndex.data(m_roleForDeduplication).toString();
            if (data.isEmpty()) {
                continue;
            }

            auto list = m_map.value(data, nullptr);
            if (nullptr == list) {
                beginInsertRows(QModelIndex(), m_rowMap.size(), m_rowMap.size());
                list = new QList<int>();
                m_map.insert(data, list);
                m_rowMap.append(list);
                endInsertRows();
            }
            beginInsertRows(index(m_rowMap.indexOf(list), 0), list->size(), list->size());
            list->append(i);
            endInsertRows();
        }
    });

    connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        for (int i = 0; i < m_rowMap.count(); ++i) {
            auto sourceRows = m_rowMap.value(i);
            for (int j = 0; j < sourceRows->size(); ++j) {
                if (first <= sourceRows->value(j) && last >= sourceRows->value(j)) {
                    beginRemoveRows(index(m_rowMap.indexOf(sourceRows), 0), j, j);
                    sourceRows->removeAt(j);
                    endRemoveRows();
                    --j;
                }
            }
            if (sourceRows->size() == 0) {
                beginRemoveRows(QModelIndex(), m_rowMap.indexOf(sourceRows), m_rowMap.indexOf(sourceRows));
                m_map.remove(m_map.key(sourceRows));
                m_rowMap.removeOne(sourceRows);
                delete sourceRows;
                endRemoveRows();
            }
        }
        adjustMap(first, -((last - first) + 1));
    });

    connect(sourceModel(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
        // TODO: if roles contains m_roleForDeduplication, need update from topLeft 2 bottomRight or just send dataChanged
        if (roles.contains(m_roleForDeduplication)) {
            rebuildTreeSource();
            return;
        }

        for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
            auto data = sourceModel()->index(i, 0).data(m_roleForDeduplication).toString();
            auto list = m_map.value(data, nullptr);
            if (list == nullptr)
                continue;

            auto index = createIndex(list->indexOf(i), 0, m_rowMap.indexOf(list));
            Q_EMIT dataChanged(index, index, roles);
        }
    });

    connect(sourceModel(), &QAbstractItemModel::modelReset, this, [this]() {
        rebuildTreeSource();
    });
}

int RoleGroupModel::rowCount(const QModelIndex &parent) const
{
    if (!sourceModel()) {
        return 0;
    }
    if (parent.isValid()) {
        auto list = m_rowMap.value(parent.row(), nullptr);
        return nullptr == list ? 0 : list->size();
    }

    return m_rowMap.size();
}

int RoleGroupModel::columnCount(const QModelIndex &parent) const
{
    if (sourceModel()) {
        return sourceModel()->columnCount(parent);
    }
    return 0;
}

QHash<int, QByteArray> RoleGroupModel::roleNames() const
{
    if (sourceModel()) {
        return sourceModel()->roleNames();
    }
    return QAbstractProxyModel::roleNames();
}

QVariant RoleGroupModel::data(const QModelIndex &index, int role) const
{
    auto parentPos = static_cast<int>(index.internalId());
    auto list = m_rowMap.value(index.row(), nullptr);
    if (parentPos == -1) {
        if (nullptr == list || list->size() == 0) {
            return QVariant();
        }
        return sourceModel()->index(list->first(), 0).data(role);
    }

    list = m_rowMap.value(parentPos);
    if (list == nullptr) {
        return QVariant();
    }

    return sourceModel()->index(list->value(index.row()), 0).data(role);
}

QModelIndex RoleGroupModel::index(int row, int column, const QModelIndex &parent) const
{
    auto list = m_map.value(parent.data(m_roleForDeduplication).toString(), nullptr);
    if (parent.isValid() && nullptr != list) {
        return createIndex(row, column, m_rowMap.indexOf(list));
    }

    return createIndex(row, column, -1);
}

QModelIndex RoleGroupModel::parent(const QModelIndex &child) const
{
    auto pos = static_cast<int>(child.internalId());
    if (pos == -1)
        return QModelIndex();

    return createIndex(pos, 0);
}

QModelIndex RoleGroupModel::mapToSource(const QModelIndex &proxyIndex) const
{
    auto parentIndex = proxyIndex.parent();
    QList<int> *list = nullptr;

    if (parentIndex.isValid()) {
        list = m_rowMap.value(parentIndex.row());
    } else {
        list = m_map.value(proxyIndex.data(m_roleForDeduplication).toString());
    }

    if (nullptr == list)
        return QModelIndex();

    if (parentIndex.isValid()) {
        return sourceModel()->index(list->value(proxyIndex.row()), 0);
    }

    return sourceModel()->index(list->value(0), 0);
}

QModelIndex RoleGroupModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    auto data = sourceIndex.data(m_roleForDeduplication).toString();
    if (data.isEmpty()) {
        return QModelIndex();
    }

    auto list = m_map.value(data, nullptr);
    if (nullptr == list || 0 == list->size()) {
        return {};
    }

    if (sourceIndex.row() == list->first()) {
        return createIndex(m_rowMap.indexOf(list), 0, -1);
    }

    auto pos = list->indexOf(sourceIndex.row());
    return createIndex(pos, 0, m_rowMap.indexOf(list));
}

void RoleGroupModel::rebuildTreeSource()
{
    beginResetModel();
    qDeleteAll(m_rowMap);
    m_map.clear();
    m_rowMap.clear();

    if (sourceModel() == nullptr) {
        endResetModel();
        return;
    }

    for (int i = 0; i < sourceModel()->rowCount(); i++) {
        auto index = sourceModel()->index(i, 0);
        auto data = index.data(m_roleForDeduplication).toString();
        if (data.isEmpty()) {
            continue;
        }

        auto list = m_map.value(data, nullptr);
        if (nullptr == list) {
            list = new QList<int>();
            m_map.insert(data, list);
            m_rowMap.append(list);
        }
        list->append(i);
    }
    endResetModel();
}

void RoleGroupModel::adjustMap(int base, int offset)
{
    for (int i = 0; i < m_rowMap.count(); ++i) {
        auto sourceRows = m_rowMap.value(i);
        for (int j = 0; j < sourceRows->size(); ++j) {
            if (sourceRows->value(j) < base)
                continue;
            (*sourceRows)[j] += offset;
        }
    }
}
