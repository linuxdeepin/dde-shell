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

    void requestActivate(const QModelIndex &index) const override;
    void requestNewInstance(const QModelIndex &index, const QString &action) const override;
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const override;
    void requestClose(const QModelIndex &index, bool force = false) const override;
    void requestUpdateWindowGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const override;
    void requestPreview(const QModelIndexList &indexes,
                        QObject *relativePositionItem,
                        int32_t previewXoffset,
                        int32_t previewYoffset,
                        uint32_t direction) const override;
    void requestWindowsView(const QModelIndexList &indexes) const override;

    void dumpItemInfo(const QModelIndex &index);

private:
    QAbstractItemModel *m_globalModel;
    QScopedPointer<QAbstractItemModel> m_groupModel;
    bool m_split;
    bool m_isUpdating;
};
}
