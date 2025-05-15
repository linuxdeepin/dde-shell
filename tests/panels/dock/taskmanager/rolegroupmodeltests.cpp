// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rolegroupmodel.h"
#include <QVariant>

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QStandardItemModel>
#include <qhash.h>

#include "rolegroupmodel.h"

TEST(RoleGroupModel, RowCountTest)
{
    QStandardItemModel model;
    QStandardItem *item1 = new QStandardItem;
    QStandardItem *item2 = new QStandardItem;
    QStandardItem *item3 = new QStandardItem;
    QStandardItem *item4 = new QStandardItem;
    QStandardItem *item5 = new QStandardItem;

    auto role = Qt::UserRole + 1;

    RoleGroupModel groupModel(&model, role);
    model.setItemRoleNames({{role, "data"}});

    item1->setData(QString("data"), role);
    model.appendRow(item1);

    item2->setData(QString("data"), role);
    model.appendRow(item2);

    model.appendRow(item3);
    model.appendRow(item4);
    model.appendRow(item5);
    QSignalSpy spy(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy spys(&model, &QAbstractItemModel::rowsInserted);

    item3->setData(QString("data3"), role);
    item4->setData(QString("data4"), role);
    item5->setData(QString("data4"), role);

    auto index1 = groupModel.index(0, 0);
    auto index11 = groupModel.index(0, 0, index1);
    auto index2 = groupModel.index(1, 0, index1);
    auto index3 = groupModel.index(1, 0);
    auto index4 = groupModel.index(2, 0);

    EXPECT_EQ(groupModel.rowCount(), 3);
    EXPECT_EQ(groupModel.rowCount(index1), 2);
    EXPECT_EQ(groupModel.rowCount(index3), 1);
    EXPECT_EQ(groupModel.rowCount(index4), 2);

    EXPECT_EQ(index11.data(role), index1.data(role));
    EXPECT_EQ(index1.data(role), "data");
    EXPECT_EQ(index2.data(role), "data");
    EXPECT_EQ(index3.data(role), "data3");
    EXPECT_EQ(index4.data(role), "data4");

    // 移除了item3
    model.removeRows(2, 1);
    EXPECT_EQ(groupModel.rowCount(), 2);

    // 移除了item4
    model.removeRows(2, 1);
    EXPECT_EQ(groupModel.rowCount(), 2);

    index4 = groupModel.index(1, 0);
    EXPECT_EQ(groupModel.rowCount(index1), 2);
    // item5 的内容为data4
    EXPECT_EQ(groupModel.rowCount(index4), 1);
}
