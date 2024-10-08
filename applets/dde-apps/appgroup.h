// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace apps {
class AppGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint id READ id FINAL CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QList<int> pages READ pages WRITE setPages NOTIFY appitemsChanged)
    Q_PROPERTY(QStringList appItems READ appItems NOTIFY appitemsChanged)

public:
    explicit AppGroup(const uint& id, QObject* parent = nullptr);
    explicit AppGroup(const uint& id, const QString &name, QObject* parent = nullptr);
    explicit AppGroup(const uint& id, const QString &name, const QStringList &appItemIDs, const QList<int> &pages, QObject* parent = nullptr);

    uint id() const;
    QString name() const;
    void setName(const QString &name);
    QStringList appItems() const;
    /*
     * itemId 之需要插入的itemId
     * pos 指的是插入的逻辑位置
     */
    void instertItem(const QString &itemId, int pos = -1);
    void removeItem(const QString& itemId);

    void swap(const QString &itemA, const QString &itemB);
    void move(const QString &item, int pos);

    QList<int> pages();
    void setPages(const QList<int> &pages);

private:
    int getPageById(const QString &appItem);

Q_SIGNALS:
    void nameChanged();
    void appitemsChanged();
    void pagesChanged();

private:
    uint m_id;
    QString m_name;
    QStringList m_itemIds;
    QList<int> m_pages;
};
}
