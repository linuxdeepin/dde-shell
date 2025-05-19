// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockcombinemodel.h"
#include "abstracttaskmanagerinterface.h"
#include "globals.h"
#include "rolecombinemodel.h"
#include "taskmanager.h"

namespace dock
{
DockCombineModel::DockCombineModel(QAbstractItemModel *major, QAbstractItemModel *minor, int majorRoles, CombineFunc func, QObject *parent)
    : RoleCombineModel(major, minor, majorRoles, func, parent)
    , AbstractTaskManagerInterface(this)
{
    // due to role has changed by RoleGroupModel, so we redirect role to TaskManager::Roles.
    m_roleMaps = {{TaskManager::ActiveRole, RoleCombineModel::roleNames().key(MODEL_ACTIVE)},
                  {TaskManager::AttentionRole, RoleCombineModel::roleNames().key(MODEL_ATTENTION)},
                  {TaskManager::DesktopIdRole, RoleCombineModel::roleNames().key(MODEL_DESKTOPID)},
                  {TaskManager::IconNameRole, RoleCombineModel::roleNames().key(MODEL_ICONNAME)},
                  {TaskManager::IdentityRole, RoleCombineModel::roleNames().key(MODEL_IDENTIFY)},
                  {TaskManager::ActionsRole, RoleCombineModel::roleNames().key(MODEL_ACTIONS)},
                  {TaskManager::NameRole, RoleCombineModel::roleNames().key(MODEL_NAME)},
                  {TaskManager::WinIdRole, RoleCombineModel::roleNames().key(MODEL_WINID)},
                  {TaskManager::WinIconRole, RoleCombineModel::roleNames().key(MODEL_WINICON)},
                  {TaskManager::WinTitleRole, RoleCombineModel::roleNames().key(MODEL_TITLE)}};
}

QHash<int, QByteArray> DockCombineModel::roleNames() const
{
    return {{TaskManager::ActiveRole, MODEL_ACTIVE},
            {TaskManager::AttentionRole, MODEL_ATTENTION},
            {TaskManager::DesktopIdRole, MODEL_DESKTOPID},
            {TaskManager::IconNameRole, MODEL_ICONNAME},
            {TaskManager::IdentityRole, MODEL_IDENTIFY},
            {TaskManager::ActionsRole, MODEL_ACTIONS},
            {TaskManager::NameRole, MODEL_NAME},
            {TaskManager::WinIdRole, MODEL_WINID},
            {TaskManager::WinIconRole, MODEL_WINICON},
            {TaskManager::WinTitleRole, MODEL_TITLE}};
}

QVariant DockCombineModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case TaskManager::DesktopIdRole: {
        auto res = RoleCombineModel::data(index, m_roleMaps.value(TaskManager::DesktopIdRole)).toString();
        if (res.isEmpty()) {
            auto data = RoleCombineModel::data(index, m_roleMaps.value(TaskManager::IdentityRole)).toStringList();
            res = data.value(0, "");
        }
        return res;
    }
    case TaskManager::IconNameRole: {
        QString winTitle = RoleCombineModel::data(index, m_roleMaps.value(TaskManager::WinIconRole)).toString();
        auto icon = RoleCombineModel::data(index, m_roleMaps.value(TaskManager::IconNameRole)).toString();
        if (icon.isEmpty()) {
            icon = RoleCombineModel::data(index, m_roleMaps.value(TaskManager::WinIconRole)).toString();
        }
        return icon;
    }
    default: {
        auto newRole = m_roleMaps.value(role, -1);
        if (newRole == -1) {
            return QVariant();
        }

        return RoleCombineModel::data(index, newRole);
    }
    }

    return QVariant();
}
}