// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockgroupmodel.h"
#include "abstracttaskmanagerinterface.h"
#include "rolegroupmodel.h"
#include "taskmanager.h"

namespace dock
{
DockGroupModel::DockGroupModel(QAbstractItemModel *sourceModel, int role, QObject *parent)
    : RoleGroupModel(sourceModel, role, parent)
    , m_roleForDeduplication(role)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int first, int last) {
        if (!parent.isValid())
            return;
        Q_EMIT dataChanged(index(parent.row(), 0), index(parent.row(), 0), {TaskManager::WindowsRole});
    });
    connect(this, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        if (!parent.isValid())
            return;
        Q_EMIT dataChanged(index(parent.row(), 0), index(parent.row(), 0), {TaskManager::WindowsRole});
    });

    connect(this, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
        if (!topLeft.parent().isValid())
            return;
        auto parentRow = topLeft.parent().row();
        Q_EMIT dataChanged(index(parentRow, 0), index(parentRow, 0), roles);

        if (roles.contains(TaskManager::ActiveRole))
            m_currentActiveWindow.insert(parentRow, topLeft.row());
    });
}

QVariant DockGroupModel::data(const QModelIndex &index, int role) const
{
    if (role == m_roleForDeduplication) {
        return RoleGroupModel::data(index, role);
    }

    if (TaskManager::WindowsRole == role) {
        QStringList stringList;
        auto variantList = all(index, role);
        std::transform(variantList.begin(), variantList.end(), std::back_inserter(stringList), [](const QVariant &var) {
            return var.toString();
        });
        return stringList;
    } else if (TaskManager::ActiveRole == role || TaskManager::AttentionRole == role) {
        return any(index, role);
    }

    return RoleGroupModel::data(index, role);
}

int DockGroupModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : RoleGroupModel::rowCount();
}

QModelIndex DockGroupModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();

    return RoleGroupModel::index(row, column);
}

bool DockGroupModel::any(const QModelIndex &index, int role) const
{
    auto rowCount = RoleGroupModel::rowCount(index);
    for (int i = 0; i < rowCount; i++) {
        auto cIndex = RoleGroupModel::index(i, 0, index);
        if (RoleGroupModel::data(cIndex, role).toBool())
            return true;
    }

    return false;
}

QVariantList DockGroupModel::all(const QModelIndex &index, int role) const
{
    QVariantList res;
    auto rowCount = RoleGroupModel::rowCount(index);
    for (int i = 0; i < rowCount; i++) {
        auto cIndex = RoleGroupModel::index(i, 0, index);
        auto window = RoleGroupModel::data(index, role);
        if (window.isValid())
            res.append(window);
    }

    return res;
}

void DockGroupModel::requestActivate(const QModelIndex &index) const
{
    auto interface = dynamic_cast<AbstractTaskManagerInterface *>(sourceModel());
    if (nullptr == interface)
        return;

    QModelIndex sourceIndex;
    if (index.data(TaskManager::ActiveRole).toBool()) {
        auto next = (m_currentActiveWindow.value(index.row()) + 1) % RoleGroupModel::rowCount(index);
        sourceIndex = mapToSource(createIndex(next, 0, index.row()));
    } else {
        auto current = m_currentActiveWindow.value(index.row());
        sourceIndex = mapToSource(createIndex(current, 0, index.row()));
    }

    interface->requestActivate(sourceIndex);
}

void DockGroupModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
}

void DockGroupModel::requestNewInstance(const QModelIndex &index, const QString &action) const
{
    callInterfaceMethod(this, index, &AbstractTaskManagerInterface::requestNewInstance, action);
}

void DockGroupModel::requestClose(const QModelIndex &index, bool force) const
{
    auto interface = dynamic_cast<AbstractTaskManagerInterface *>(sourceModel());
    if (nullptr == interface)
        return;

    auto indexRowCount = RoleGroupModel::rowCount(index);
    for (int i = 0; i < indexRowCount; i++) {
        auto cIndex = RoleGroupModel::index(i, 0, index);
        callInterfaceMethod(this, cIndex, &AbstractTaskManagerInterface::requestClose, force);
    }
}

void DockGroupModel::requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
}

void DockGroupModel::requestPreview(const QModelIndexList &indexes,
                                    QObject *relativePositionItem,
                                    int32_t previewXoffset,
                                    int32_t previewYoffset,
                                    uint32_t direction) const
{
    QModelIndexList proxyIndexes;
    for (auto index : indexes) {
        for (int i = 0; i < RoleGroupModel::rowCount(index); ++i) {
            auto proxyIndex = createIndex(i, 0, index.row());
            proxyIndexes.append(proxyIndex);
        }
    }

    callInterfaceMethod(this, proxyIndexes, &AbstractTaskManagerInterface::requestPreview, relativePositionItem, previewXoffset, previewYoffset, direction);
}

void DockGroupModel::requestWindowsView(const QModelIndexList &indexes) const
{
    QModelIndexList sourceIndexes;
    for (auto index : indexes) {
        for (int i = 0; i < RoleGroupModel::rowCount(index); i++) {
            sourceIndexes.append(mapToSource(createIndex(i, 0, index.row())));
        }
    }

    callInterfaceMethod(this, sourceIndexes, &AbstractTaskManagerInterface::requestWindowsView);
}
}