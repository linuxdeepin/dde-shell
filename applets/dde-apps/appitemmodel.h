// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitem.h"
#include <QAbstractListModel>

namespace apps {
class AppItemModel : public QAbstractListModel
{
    Q_OBJECT

public:
    AppItemModel(QObject* parent);
    ~AppItemModel();
    enum Roles {
        DesktopIdRole = Qt::UserRole + 1,
        NameRole,
        IconNameRole,
        StartUpWMClassRole,
        NoDisplayRole,
        ActionsRole,
        DDECategoryRole,
        InstalledTimeRole,
        LastLaunchedTimeRole,
        LaunchedTimesRole,
        DockedRole,
        OnDesktopRole,
        AutoStartRole,
        ModelExtendedRole = 0x1000
    };
    Q_ENUM(Roles)

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = DesktopIdRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = DockedRole) override;

    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

    void addAppItems(const QList<AppItem*> &item);
    void removeAppItems(const QList<AppItem*> &items);

private:
    QList<AppItem*> m_items;
};

}
