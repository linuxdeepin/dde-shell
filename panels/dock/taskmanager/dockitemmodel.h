// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstracttaskmanagerinterface.h"

#include <QAbstractProxyModel>

namespace dock
{
class DockItemModel : public QAbstractProxyModel, public AbstractTaskManagerInterface
{
public:
    explicit DockItemModel(QAbstractItemModel *globalModel, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setSourceModel(QAbstractItemModel *model) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

    void dumpItemInfo(const QModelIndex &index);

private:
    QAbstractItemModel *m_globalModel;
    QScopedPointer<QAbstractItemModel> m_groupModel;
    bool m_split;
    bool m_isUpdating;
};
}
