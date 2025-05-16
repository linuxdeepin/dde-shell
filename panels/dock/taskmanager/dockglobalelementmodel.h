// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "abstracttaskmanagerinterface.h"
#include "dockcombinemodel.h"
#include <QAbstractProxyModel>
#include <QPointer>
#include <tuple>

namespace dock
{
class DockGlobalElementModel : public QAbstractListModel, public AbstractTaskManagerInterface
{
    Q_OBJECT
public:
    explicit DockGlobalElementModel(QAbstractItemModel *appsModel, DockCombineModel *activeAppModel, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    inline int mapToSourceModelRole(QAbstractItemModel *model, int role) const;

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
    void loadDockedElements();
    QString getMenus(const QModelIndex &index) const;

private:
    // id, model, and pos
    QList<std::tuple<QString, QAbstractItemModel *, int>> m_data;

    // type, id
    QList<std::tuple<QString, QString>> m_dockedElements;
    QAbstractItemModel *m_appsModel;
    DockCombineModel *m_activeAppModel;
};
}
