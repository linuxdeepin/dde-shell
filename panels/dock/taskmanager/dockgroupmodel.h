// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "abstracttaskmanagerinterface.h"
#include "rolegroupmodel.h"

namespace dock
{
class DockGroupModel : public RoleGroupModel, public AbstractTaskManagerInterface
{
    Q_OBJECT

public:
    explicit DockGroupModel(QAbstractItemModel *sourceModel, int role, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    Q_INVOKABLE virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

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

private:
    bool any(const QModelIndex &index, int role) const;
    QVariantList all(const QModelIndex &index, int role) const;

private:
    int m_roleForDeduplication;
    QHash<int, int> m_currentActiveWindow;
};
}
