// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rolecombinemodel.h"

#include <algorithm>

RoleCombineModel::RoleCombineModel(QAbstractItemModel* major, QAbstractItemModel* minor, int majorRoles, CombineFunc func, QObject* parent)
    : QAbstractItemModel(parent)
    , m_major(major)
    , m_minor(minor)
{
    // create minor row & column map
    int rowCount = m_major->rowCount();
    int columnCount = m_major->columnCount();
    for (int i = 0; i < rowCount; i++) {
        for (int j = 0; j < columnCount; j++) {
            QModelIndex majorIndex = major->index(i, j);
            QModelIndex minorIndex = func(majorIndex.data(majorRoles), m_minor);
            if (majorIndex.isValid() && minorIndex.isValid())
                m_indexMap[qMakePair(i, j)] = qMakePair(minorIndex.row(), minorIndex.column());
        }
    }

    // connect changedSignal
    connect(m_major, &QAbstractItemModel::dataChanged, this,
        [this, majorRoles, func](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles){
            for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
                for (int j = topLeft.column(); j <= bottomRight.column(); j++) {
                    QModelIndex majorIndex = m_major->index(i, j);
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
            for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
                for (int j =  topLeft.column(); j <= bottomRight.column(); j++) {
                    auto majorPos = m_indexMap.key(qMakePair(i, j), qMakePair(-1, -1));
                    if (-1 == majorPos.first && -1 == majorPos.second)
                        continue;

                    auto majorIndex = m_major->index(majorPos.first, majorPos.second);
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

    // create minor role map call it at next evnet loop
    QMetaObject::invokeMethod(this, [this](){
        auto minorRolenames = m_minor->roleNames();
        auto thisRoleNames = roleNames();
        std::for_each(minorRolenames.constBegin(), minorRolenames.constEnd(), [&minorRolenames, &thisRoleNames, this](auto &roleName){
            m_minorRolesMap.insert(thisRoleNames.key(roleName), minorRolenames.key(roleName));
        });
    }, Qt::QueuedConnection);
}

QHash<int, QByteArray> RoleCombineModel::roleNames() const
{
    auto roleNames = m_major->roleNames();
    auto keys = m_major->roleNames().keys();
    auto lastRole = *(std::max_element(keys.constBegin(), keys.constEnd()));
    auto minorRoleNames = m_minor->roleNames().values();
    std::for_each(minorRoleNames.constBegin(), minorRoleNames.constEnd(), [&lastRole, &roleNames, this](auto &roleName){
        roleNames.insert(++lastRole, roleName);
    });

    return roleNames;
}

int RoleCombineModel::rowCount(const QModelIndex &parent) const
{
    return m_major->rowCount();
}

int RoleCombineModel::columnCount(const QModelIndex &parent) const
{
    return m_major->columnCount();
}

QVariant RoleCombineModel::data(const QModelIndex &index, int role) const
{
    if (m_minorRolesMap.contains(role)) {
        int row, column;
        std::tie(row, column) = m_indexMap.value(qMakePair(index.row(), index.column()));
        return m_minor->data(m_minor->index(row, column), m_minorRolesMap[role]);
    } else {
        return m_major->data(m_major->index(index.row(), index.column()), role);
    }
}

bool RoleCombineModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return m_major->hasIndex(row, column, parent);
}

QModelIndex RoleCombineModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto index = m_major->index(row, column);
    return createIndex(row, column, index.internalPointer());
}

QModelIndex RoleCombineModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QAbstractItemModel* RoleCombineModel::majorModel() const
{
    return m_major;
}

QAbstractItemModel* RoleCombineModel::minorModel() const
{
    return m_minor;
}
