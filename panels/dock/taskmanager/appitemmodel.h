// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QAbstractListModel>

DS_BEGIN_NAMESPACE
namespace dock {
class AppItem;

class AppItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ItemIdRole = Qt::UserRole + 1,
        NameRole,
        IconNameRole,
        ActiveRole,
        MenusRole,
        DockedRole,
        WindowsRole,
    };
    Q_ENUM(Roles)

    static AppItemModel* instance();
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role = ItemIdRole) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    // for AppItemFactory to get already created AppItem;
    QPointer<AppItem> getAppItemById(const QString& id) const;

    // for AppItemFactory to add new AppItem
    void addAppItem(QPointer<AppItem> item);

Q_SIGNALS:
    void appItemAdded();
    void appItemRemoved();

private Q_SLOTS:
    void onAppItemDestroyed();
    void onAppItemChanged();

private:
    explicit AppItemModel(QObject* parent = nullptr);

    int m_recentSize;
    QList<QPointer<AppItem>> m_appItems;
};
}
DS_END_NAMESPACE
