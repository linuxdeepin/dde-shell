// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractProxyModel>

class RoleGroupModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    explicit RoleGroupModel(QAbstractItemModel *sourceModel, int role, QObject *parent = nullptr);
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    void setDeduplicationRole(const int &role);
    int deduplicationRole() const;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool hasIndex(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    Q_INVOKABLE virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE virtual QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

Q_SIGNALS:
    void deduplicationRoleChanged(int role);

private:
    void rebuildTreeSource();
    void adjustMap(int base, int offset);

private:
    int m_roleForDeduplication;

    // for order
    QList<QString> m_rowMap;

    // data 2 source row
    QHash<QString, QList<int>> m_map;
};
