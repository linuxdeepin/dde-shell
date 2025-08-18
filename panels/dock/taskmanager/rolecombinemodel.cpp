// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rolecombinemodel.h"

#include <algorithm>

RoleCombineModel::RoleCombineModel(QAbstractItemModel* major, QAbstractItemModel* minor, int majorRoles, CombineFunc func, QObject* parent)
    : QAbstractProxyModel(parent)
{
    setSourceModel(major);
    m_minor = minor;
    // create minor row & column map
    int rowCount = major->rowCount();
    int columnCount = major->columnCount();
    for (int i = 0; i < rowCount; i++) {
        for (int j = 0; j < columnCount; j++) {
            QModelIndex majorIndex = major->index(i, j);
            QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
            if (majorIndex.isValid() && minorIndex.isValid())
                m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
        }
    }

    connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, [this, majorRoles, func](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        // 对于QAbstractListModel，parent通常是无效的，我们直接使用QModelIndex()
        beginInsertRows(QModelIndex(), first, last);

        // 先调整现有映射中后续行的索引
        QMap<QPair<int, int>, QPair<int, int>> newIndexMap;
        for (auto it = m_indexMap.constBegin(); it != m_indexMap.constEnd(); ++it) {
            int row = it.key().first;
            int col = it.key().second;

            if (row >= first) {
                // 将后续行的索引向后移动
                int newRow = row + (last - first + 1);
                newIndexMap[qMakePair(newRow, col)] = it.value();
            } else {
                // 保持前面行的映射不变
                newIndexMap[it.key()] = it.value();
            }
        }
        m_indexMap = newIndexMap;

        // 为新插入的行创建映射
        int columnCount = sourceModel()->columnCount();
        for (int i = first; i <= last; i++) {
            for (int j = 0; j < columnCount; j++) {
                QModelIndex majorIndex = sourceModel()->index(i, j);
                QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
                if (majorIndex.isValid() && minorIndex.isValid())
                    m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
            }
        }
        endInsertRows();
    });

    connect(sourceModel(), &QAbstractItemModel::columnsInserted, this, [this, majorRoles, func](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        beginInsertColumns(QModelIndex(), first, last);

        // 先调整现有映射中后续列的索引
        QMap<QPair<int, int>, QPair<int, int>> newIndexMap;
        for (auto it = m_indexMap.constBegin(); it != m_indexMap.constEnd(); ++it) {
            int row = it.key().first;
            int col = it.key().second;

            if (col >= first) {
                // 将后续列的索引向后移动
                int newCol = col + (last - first + 1);
                newIndexMap[qMakePair(row, newCol)] = it.value();
            } else {
                // 保持前面列的映射不变
                newIndexMap[it.key()] = it.value();
            }
        }
        m_indexMap = newIndexMap;

        // 为新插入的列创建映射
        int rowCount = sourceModel()->rowCount();
        for (int j = first; j <= last; j++) {
            for (int i = 0; i < rowCount; i++) {
                QModelIndex majorIndex = sourceModel()->index(i, j);
                QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
                if (majorIndex.isValid() && minorIndex.isValid())
                    m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
            }
        }
        endInsertColumns();
    });

    connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        beginRemoveRows(QModelIndex(), first, last);

        // 删除被移除行的映射
        int columnCount = sourceModel()->columnCount();
        for (int i = first; i <= last; i++) {
            for (int j = 0; j < columnCount; j++) {
                m_indexMap.remove(qMakePair(i, j));
            }
        }

        // 调整后续行的索引映射
        QMap<QPair<int, int>, QPair<int, int>> newIndexMap;
        for (auto it = m_indexMap.constBegin(); it != m_indexMap.constEnd(); ++it) {
            int row = it.key().first;
            int col = it.key().second;

            if (row > last) {
                // 将后续行的索引向前移动
                int newRow = row - (last - first + 1);
                newIndexMap[qMakePair(newRow, col)] = it.value();
            } else if (row < first) {
                // 保持前面行的映射不变
                newIndexMap[it.key()] = it.value();
            }
            // 被删除行的映射已经在上面移除了
        }
        m_indexMap = newIndexMap;

        endRemoveRows();
    });

    connect(sourceModel(), &QAbstractItemModel::columnsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        beginRemoveColumns(QModelIndex(), first, last);

        // 删除被移除列的映射
        int rowCount = sourceModel()->rowCount();
        for (int j = first; j <= last; j++) {
            for (int i = 0; i < rowCount; i++) {
                m_indexMap.remove(qMakePair(i, j));
            }
        }

        // 调整后续列的索引映射
        QMap<QPair<int, int>, QPair<int, int>> newIndexMap;
        for (auto it = m_indexMap.constBegin(); it != m_indexMap.constEnd(); ++it) {
            int row = it.key().first;
            int col = it.key().second;

            if (col > last) {
                // 将后续列的索引向前移动
                int newCol = col - (last - first + 1);
                newIndexMap[qMakePair(row, newCol)] = it.value();
            } else if (col < first) {
                // 保持前面列的映射不变
                newIndexMap[it.key()] = it.value();
            }
            // 被删除列的映射已经在上面移除了
        }
        m_indexMap = newIndexMap;

        endRemoveColumns();
    });

    // connect changedSignal
    connect(major, &QAbstractItemModel::dataChanged, this,
        [this, majorRoles, func](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles){
            Q_UNUSED(roles)
            for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
                for (int j = topLeft.column(); j <= bottomRight.column(); j++) {
                    QModelIndex majorIndex = sourceModel()->index(i, j);
                    QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
                    if (majorIndex.isValid() && minorIndex.isValid())
                        m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
                }
            }

            Q_EMIT dataChanged(index(topLeft.row(), topLeft.column()),
                index(bottomRight.row(), bottomRight.column()),
                roles + (roles.contains(majorRoles) ? m_minorRolesMap.values() : QList<int>())
            );
    });

    // appended roles from minor datachanged
    connect(m_minor, &QAbstractItemModel::dataChanged, this,
        [this, majorRoles, func](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles){
            Q_UNUSED(roles)
            for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
                for (int j =  topLeft.column(); j <= bottomRight.column(); j++) {
                    auto majorPos = m_indexMap.key(qMakePair(i, j), qMakePair(-1, -1));
                    if (-1 == majorPos.first && -1 == majorPos.second)
                        continue;

                    auto majorIndex = sourceModel()->index(majorPos.first, majorPos.second);
                    if (!majorIndex.isValid())
                        continue;

                    auto minorIndex = func(majorIndex.data(majorRoles), m_minor);
                    if (!minorIndex.isValid())
                        continue;

                    m_indexMap[majorPos] = qMakePair(minorIndex.row(), minorIndex.column());
                    Q_EMIT dataChanged(majorIndex, majorIndex, m_minorRolesMap.values());
                }
            }
    });

    // 添加对minor模型删除操作的处理
    connect(m_minor, &QAbstractItemModel::rowsRemoved, this, [this, majorRoles, func](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        // 当minor模型删除行时，需要更新映射并可能触发数据变化信号
        QList<QModelIndex> affectedMajorIndexes;

        // 找到受影响的major索引
        for (auto it = m_indexMap.begin(); it != m_indexMap.end();) {
            int minorRow = it.value().first;

            if (minorRow >= first && minorRow <= last) {
                // 这个映射指向的minor行被删除了，需要重新建立映射
                int majorRow = it.key().first;
                int majorCol = it.key().second;
                auto majorIndex = sourceModel()->index(majorRow, majorCol);

                if (majorIndex.isValid()) {
                    affectedMajorIndexes.append(majorIndex);
                    // 尝试重新建立映射
                    QModelIndex newMinorIndex = func(majorIndex.data(majorRoles), m_minor);
                    if (newMinorIndex.isValid()) {
                        it.value() = qMakePair(newMinorIndex.row(), newMinorIndex.column());
                        ++it;
                    } else {
                        // 无法建立新映射，删除这个映射
                        it = m_indexMap.erase(it);
                    }
                } else {
                    it = m_indexMap.erase(it);
                }
            } else if (minorRow > last) {
                // 调整后续行的索引
                int newMinorRow = minorRow - (last - first + 1);
                it.value().first = newMinorRow;
                ++it;
            } else {
                ++it;
            }
        }

        // 对受影响的major索引发送数据变化信号
        for (const auto &majorIndex : affectedMajorIndexes) {
            Q_EMIT dataChanged(majorIndex, majorIndex, m_minorRolesMap.values());
        }
    });

    connect(m_minor, &QAbstractItemModel::columnsRemoved, this, [this, majorRoles, func](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent)
        // 当minor模型删除列时，需要更新映射
        for (auto it = m_indexMap.begin(); it != m_indexMap.end(); ++it) {
            int minorCol = it.value().second;

            if (minorCol > last) {
                // 调整后续列的索引
                int newMinorCol = minorCol - (last - first + 1);
                it.value().second = newMinorCol;
            }
        }
    });

    connect(m_minor, &QAbstractItemModel::rowsInserted, this,
        [this, majorRoles, func](const QModelIndex &parent, int first, int last){
            Q_UNUSED(parent)
            Q_UNUSED(first)
            Q_UNUSED(last)
        auto rowCount = sourceModel()->rowCount();
        auto columnCount = sourceModel()->columnCount();
        for (int i = 0; i < rowCount; i++) {
            for (int j = 0; j < columnCount; j++) {
                // already bind, pass this
                if (m_indexMap.contains(qMakePair(i ,j)))
                    continue;

                QModelIndex majorIndex = sourceModel()->index(i, j);
                QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
                if (majorIndex.isValid() && minorIndex.isValid())
                    m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
            }
        }
    });

    // TODO: support columsInserted

    // create minor role map
    auto minorRolenames = m_minor->roleNames();
    m_roleNames = createRoleNames();

    // 修复角色映射逻辑：应该映射到新创建的minor角色，而不是major角色
    auto majorRoleNames = sourceModel()->roleNames();

    std::for_each(minorRolenames.constBegin(), minorRolenames.constEnd(), [&minorRolenames, &majorRoleNames, this](auto &roleName) {
        int minorRoleKey = minorRolenames.key(roleName);

        // 在组合角色中找到对应的key，但排除major模型已有的key
        int combinedRoleKey = -1;
        for (auto it = m_roleNames.constBegin(); it != m_roleNames.constEnd(); ++it) {
            if (it.value() == roleName && !majorRoleNames.contains(it.key())) {
                combinedRoleKey = it.key();
                break;
            }
        }

        if (combinedRoleKey != -1) {
            m_minorRolesMap.insert(combinedRoleKey, minorRoleKey);
        }
    });
}

QHash<int, QByteArray> RoleCombineModel::createRoleNames() const
{
    auto roleNames = sourceModel()->roleNames();
    auto keys = sourceModel()->roleNames().keys();
    auto lastRole = *(std::max_element(keys.constBegin(), keys.constEnd()));
    auto minorRoleNames = m_minor->roleNames().values();
    std::for_each(minorRoleNames.constBegin(), minorRoleNames.constEnd(), [&lastRole, &roleNames, this](auto &roleName) {
        roleNames.insert(++lastRole, roleName);
    });

    return roleNames;
}

QHash<int, QByteArray> RoleCombineModel::roleNames() const
{
    return m_roleNames;
}

int RoleCombineModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0; // 平坦列表模型：有效的parent表示某个项目，项目没有子项
    return sourceModel()->rowCount();
}

int RoleCombineModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0; // 平坦列表模型：有效的parent表示某个项目，项目没有子项
    return sourceModel()->columnCount();
}

QVariant RoleCombineModel::data(const QModelIndex &index, int role) const
{
    if (m_minorRolesMap.contains(role)) {
        auto majorKey = qMakePair(index.row(), index.column());
        auto mapping = m_indexMap.value(majorKey, qMakePair(-1, -1));
        int row = mapping.first;
        int column = mapping.second;

        // 检查映射是否有效
        if (row == -1 || column == -1) {
            return QVariant(); // 返回空值而不是用无效索引访问
        }

        return m_minor->data(m_minor->index(row, column), m_minorRolesMap[role]);
    } else {
        return sourceModel()->data(sourceModel()->index(index.row(), index.column()), role);
    }
}

bool RoleCombineModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return sourceModel()->hasIndex(row, column, parent);
}

bool RoleCombineModel::hasChildren(const QModelIndex &parent) const
{
    // 平坦列表模型：只有根节点有子项，其他项目都没有子项
    return !parent.isValid() && rowCount(parent) > 0;
}

QModelIndex RoleCombineModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto index = sourceModel()->index(row, column);
    return createIndex(row, column, index.internalPointer());
}

QModelIndex RoleCombineModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

QAbstractItemModel* RoleCombineModel::majorModel() const
{
    return sourceModel();
}

QAbstractItemModel* RoleCombineModel::minorModel() const
{
    return m_minor;
}

QModelIndex RoleCombineModel::mapToSource(const QModelIndex &proxyIndex) const
{
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column());
}

QModelIndex RoleCombineModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    return index(sourceIndex.row(), sourceIndex.column());
}
