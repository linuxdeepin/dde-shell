// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

namespace dock {
class AbstractItem;

class ItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ItemIdRole = Qt::UserRole + 64,
        // data type
        DockedDirRole,
    };
    Q_ENUM(Roles)

    static ItemModel* instance();
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role = ItemIdRole) const Q_DECL_OVERRIDE;

    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    QPointer<AbstractItem> getItemById(const QString& id) const;
    void addItem(QPointer<AbstractItem> item);
    QJsonArray dumpDockedItems() const;

    void requestActivate(const QModelIndex &index) const;
    void requestNewInstance(const QModelIndex &index, const QString &action) const;
    void requestClose(const QModelIndex &index, bool force = false) const;
    void requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const;
    void requestWindowsView(const QModelIndexList &indexes) const;
    void requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate = nullptr) const;

private Q_SLOTS:
    void onItemDestroyed();
    void onItemChanged();

private:
    explicit ItemModel(QObject* parent = nullptr);

    int m_recentSize;
    QList<QPointer<AbstractItem>> m_items;
};
}
