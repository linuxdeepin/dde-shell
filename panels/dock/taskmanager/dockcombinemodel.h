// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstracttaskmanagerinterface.h"
#include "rolecombinemodel.h"

namespace dock
{
class DockCombineModel : public RoleCombineModel, public AbstractTaskManagerInterface
{
    Q_OBJECT

public:
    DockCombineModel(QAbstractItemModel *major, QAbstractItemModel *minor, int majorRoles, CombineFunc func, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QHash<int, int> m_roleMaps;
};
}
