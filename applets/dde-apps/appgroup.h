// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStandardItem>

namespace apps
{
class AppGroup : public QStandardItem
{
public:
    explicit AppGroup(const QString &name, const QList<QStringList> &appItemIDs);

    QString name() const;
    void setName(const QString &name);

    QList<QStringList> appItems() const;
    void setAppItems(const QList<QStringList> &items);
};
}
