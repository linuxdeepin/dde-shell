// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "appitem.h"

namespace apps {
class AppGroup : public AppItem
{
public:
    explicit AppGroup(const QString &groupId, const QString &name, const QList<QStringList> &appItemIDs);

    QString name() const;
    void setName(const QString &name);

    QList<QStringList> appItems() const;
    void setAppItems(const QList<QStringList> &items);

    void setItemsPerPage(int number);
};
}
