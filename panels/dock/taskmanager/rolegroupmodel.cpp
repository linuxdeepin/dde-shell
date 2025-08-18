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
        Q_UNUSED(parent)
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
                list->append(i);
                m_map.insert(data, list);
                m_rowMap.append(list);
                endInsertRows();
            } else {
                beginInsertRows(index(m_rowMap.indexOf(list), 0), list->size(), list->size());
                list->append(i);
                endInsertRows();
            }
        }
    });

    connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
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

            int childRow = list->indexOf(i);
            if (childRow >= 0) {
                auto index = createIndex(childRow, 0, m_rowMap.indexOf(list));
                Q_EMIT dataChanged(index, index, roles);
            }
        }
    });

    connect(sourceModel(), &QAbstractItemModel::modelReset, this, [this]() {
        rebuildTreeSource();
    });
}

int RoleGroupModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        int parentRow = parent.row();
        if (parentRow < 0 || parentRow >= m_rowMap.size()) {
            return 0;
        }

        auto list = m_rowMap.value(parentRow, nullptr);
        return (list != nullptr) ? list->size() : 0;
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

bool RoleGroupModel::hasChildren(const QModelIndex &parent) const
{
    if (!sourceModel()) {
        return false;
    }

    if (!parent.isValid()) {
        // 根节点：如果有分组则有子节点
        return m_rowMap.size() > 0;
    }

    auto parentPos = static_cast<int>(parent.internalId());
    if (parentPos == -1) {
        // 这是分组节点：检查是否有子项
        if (parent.row() < 0 || parent.row() >= m_rowMap.size()) {
            return false;
        }
        auto list = m_rowMap.value(parent.row(), nullptr);
        return list && list->size() > 0;
    }

    // 这是子项：没有子节点
    return false;
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
    if (!index.isValid()) {
        return QVariant();
    }

    auto parentPos = static_cast<int>(index.internalId());

    if (parentPos == -1) {
        // 这是分组节点（顶级项目）
        auto list = m_rowMap.value(index.row(), nullptr);
        if (nullptr == list || list->isEmpty()) {
            return QVariant();
        }

        // 对于分组节点，显示分组信息和数量
        if (role == Qt::DisplayRole) {
            QString groupValue = sourceModel()->index(list->first(), 0).data(m_roleForDeduplication).toString();
            return QString("%1 (%2)").arg(groupValue).arg(list->size());
        } else {
            // 对于其他角色，返回分组中第一个项目的数据
            return sourceModel()->index(list->first(), 0).data(role);
        }
    } else {
        // 这是子项目
        auto list = m_rowMap.value(parentPos);
        if (list == nullptr || index.row() < 0 || index.row() >= list->size()) {
            return QVariant();
        }

        // 直接返回对应源项目的数据
        return sourceModel()->index(list->value(index.row()), 0).data(role);
    }
}

QModelIndex RoleGroupModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        int parentRow = parent.row();
        if (parentRow < 0 || parentRow >= m_rowMap.size()) {
            return QModelIndex();
        }

        auto list = m_rowMap.value(parentRow);
        if (nullptr == list || row < 0 || row >= list->size()) {
            return QModelIndex();
        }

        return createIndex(row, column, parentRow);
    } else {
        if (row < 0 || row >= m_rowMap.size()) {
            return QModelIndex();
        }

        return createIndex(row, column, -1);
    }
}

QModelIndex RoleGroupModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    auto pos = static_cast<int>(child.internalId());
    if (pos == -1)
        return QModelIndex();

    if (pos < 0 || pos >= m_rowMap.size()) {
        return QModelIndex();
    }

    return createIndex(pos, 0, -1);
}

QModelIndex RoleGroupModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid()) {
        return QModelIndex();
    }

    auto parentIndex = proxyIndex.parent();
    QList<int> *list = nullptr;

    if (parentIndex.isValid()) {
        int parentRow = parentIndex.row();
        if (parentRow < 0 || parentRow >= m_rowMap.size()) {
            return QModelIndex();
        }
        list = m_rowMap.value(parentRow);
        if (!list || proxyIndex.row() >= list->size()) {
            return QModelIndex();
        }
        return sourceModel()->index(list->value(proxyIndex.row()), 0);
    } else {
        list = m_map.value(proxyIndex.data(m_roleForDeduplication).toString());
        if (!list || list->isEmpty()) {
            return QModelIndex();
        }
        return sourceModel()->index(list->value(0), 0);
    }
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
    if (pos >= 0) {
        return createIndex(pos, 0, m_rowMap.indexOf(list));
    }

    return QModelIndex();
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
