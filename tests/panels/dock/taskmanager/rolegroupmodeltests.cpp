// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rolegroupmodel.h"
#include <QVariant>

#include <gtest/gtest.h>

#include <QAbstractItemModelTester>
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

// 测试 index() 方法的逻辑错误 - 这个测试实际发现了模型中的问题
TEST(RoleGroupModel, IndexMethodDeepTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 添加测试数据
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("group1"), role);
    model.appendRow(item1);

    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("group1"), role);
    model.appendRow(item2);

    // 获取有效的父索引
    auto validParent = groupModel.index(0, 0);
    EXPECT_TRUE(validParent.isValid());

    // 测试有效父索引的子索引创建
    auto validChild = groupModel.index(0, 0, validParent);
    EXPECT_TRUE(validChild.isValid());

    // 测试超出范围的顶级索引
    QModelIndex outOfRangeTopLevel = groupModel.index(999, 0);
    EXPECT_FALSE(outOfRangeTopLevel.isValid());

    // 测试超出范围的子索引
    auto outOfRangeChild = groupModel.index(999, 0, validParent);
    EXPECT_FALSE(outOfRangeChild.isValid());

    // 测试有效父索引但获取空数据的情况
    QStandardItem *itemWithEmptyData = new QStandardItem;
    // 不设置 role 数据，让其为空
    model.appendRow(itemWithEmptyData);

    // 这应该不会创建新的组，因为数据为空
    EXPECT_EQ(groupModel.rowCount(), 1); // 仍然只有1个组

    // 现在测试一个边界情况：当父索引存在但data为空时的子索引创建
    // 这种情况下应该返回无效索引，因为找不到对应的列表

    // 创建一个有效的父索引（group1），但尝试创建一个数据为空的子索引
    QStandardItem *itemWithEmptyRole = new QStandardItem;
    itemWithEmptyRole->setData(QString(""), role); // 空字符串数据
    model.appendRow(itemWithEmptyRole);

    // 由于空字符串数据会被跳过，分组数量应该仍然是1
    EXPECT_EQ(groupModel.rowCount(), 1);

    // 测试尝试为不存在的分组创建子索引
    // 构造一个看似有效但实际无效的父索引
    QModelIndex invalidParentWithValidData = groupModel.index(0, 0);
    if (invalidParentWithValidData.isValid()) {
        // 尝试获取过多的子项 - 应该返回无效索引
        int childCount = groupModel.rowCount(invalidParentWithValidData);
        QModelIndex invalidChild = groupModel.index(childCount + 10, 0, invalidParentWithValidData);
        EXPECT_FALSE(invalidChild.isValid());

        // 测试负数索引的子项
        QModelIndex negativeChild = groupModel.index(-1, 0, invalidParentWithValidData);
        EXPECT_FALSE(negativeChild.isValid());
    }
}

// 使用 QAbstractItemModelTester 进行全面的模型验证测试
TEST(RoleGroupModel, ModelTesterValidation)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 创建一个 QAbstractItemModelTester 来验证模型的正确性
    QAbstractItemModelTester tester(&groupModel, QAbstractItemModelTester::FailureReportingMode::Fatal);

    // 添加一些测试数据
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("group1"), role);
    model.appendRow(item1);

    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("group1"), role);
    model.appendRow(item2);

    QStandardItem *item3 = new QStandardItem;
    item3->setData(QString("group2"), role);
    model.appendRow(item3);

    QStandardItem *item4 = new QStandardItem;
    item4->setData(QString("group2"), role);
    model.appendRow(item4);

    // 测试数据修改
    item1->setData(QString("modified_group1"), role);

    // 测试行删除
    model.removeRow(1);

    // 测试行插入
    QStandardItem *item5 = new QStandardItem;
    item5->setData(QString("group3"), role);
    model.appendRow(item5);

    // 测试修改分组相关的数据（这会触发重建）
    item3->setData(QString("group1"), role);

    // 如果模型有任何违反Qt模型/视图架构约定的行为，
    // QAbstractItemModelTester 会抛出异常或断言失败

    // 验证最终状态
    EXPECT_GT(groupModel.rowCount(), 0);

    // 验证所有索引都是有效的
    for (int i = 0; i < groupModel.rowCount(); ++i) {
        auto index = groupModel.index(i, 0);
        EXPECT_TRUE(index.isValid());

        // 验证每个组的子项
        for (int j = 0; j < groupModel.rowCount(index); ++j) {
            auto childIndex = groupModel.index(j, 0, index);
            EXPECT_TRUE(childIndex.isValid());

            // 验证映射一致性
            auto sourceIndex = groupModel.mapToSource(childIndex);
            EXPECT_TRUE(sourceIndex.isValid());

            auto mappedBack = groupModel.mapFromSource(sourceIndex);
            EXPECT_TRUE(mappedBack.isValid());
        }
    }
}

// 测试 hasChildren 方法的正确性
TEST(RoleGroupModel, HasChildrenTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 初始状态：没有数据，根节点不应该有子节点
    EXPECT_FALSE(groupModel.hasChildren());
    EXPECT_FALSE(groupModel.hasChildren(QModelIndex()));

    // 添加第一个项目
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("firefox.desktop"), role);
    item1->setData(QString("Firefox - 页面1"), Qt::DisplayRole);
    model.appendRow(item1);

    // 现在根节点应该有子节点（分组节点）
    EXPECT_TRUE(groupModel.hasChildren());
    EXPECT_TRUE(groupModel.hasChildren(QModelIndex()));

    // 获取第一个分组节点
    QModelIndex firstGroup = groupModel.index(0, 0);
    ASSERT_TRUE(firstGroup.isValid());

    // 分组节点应该有子节点（实际的项目）
    EXPECT_TRUE(groupModel.hasChildren(firstGroup));

    // 添加同组的第二个项目
    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("firefox.desktop"), role);
    item2->setData(QString("Firefox - 页面2"), Qt::DisplayRole);
    model.appendRow(item2);

    // 分组节点仍然应该有子节点，现在有2个
    EXPECT_TRUE(groupModel.hasChildren(firstGroup));
    EXPECT_EQ(groupModel.rowCount(firstGroup), 2);

    // 获取子项目节点
    QModelIndex firstChild = groupModel.index(0, 0, firstGroup);
    ASSERT_TRUE(firstChild.isValid());

    // 子项目不应该有子节点
    EXPECT_FALSE(groupModel.hasChildren(firstChild));

    QModelIndex secondChild = groupModel.index(1, 0, firstGroup);
    ASSERT_TRUE(secondChild.isValid());
    EXPECT_FALSE(groupModel.hasChildren(secondChild));

    // 添加不同分组的项目
    QStandardItem *item3 = new QStandardItem;
    item3->setData(QString("code.desktop"), role);
    item3->setData(QString("VSCode - 项目1"), Qt::DisplayRole);
    model.appendRow(item3);

    // 现在应该有2个分组
    EXPECT_EQ(groupModel.rowCount(), 2);

    QModelIndex secondGroup = groupModel.index(1, 0);
    ASSERT_TRUE(secondGroup.isValid());
    EXPECT_TRUE(groupModel.hasChildren(secondGroup));

    // 测试空数据的处理
    QStandardItem *itemEmpty = new QStandardItem;
    itemEmpty->setData(QString(""), role);
    model.appendRow(itemEmpty);

    // 空数据应该被跳过，分组数量不变
    EXPECT_EQ(groupModel.rowCount(), 2);

    // 清空所有数据
    model.clear();

    // 清空后根节点不应该有子节点
    EXPECT_FALSE(groupModel.hasChildren());
    EXPECT_EQ(groupModel.rowCount(), 0);
}

// 测试大量索引访问的边界情况（模拟滚动场景）
TEST(RoleGroupModel, ScrollingBoundaryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 添加测试数据
    for (int i = 0; i < 5; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("app%1.desktop").arg(i % 3), role);
        item->setData(QString("Window %1").arg(i), Qt::DisplayRole);
        model.appendRow(item);
    }

    int groupCount = groupModel.rowCount();
    EXPECT_EQ(groupCount, 3);

    // 测试大量越界访问（模拟滚动到底部的场景）
    for (int i = 0; i < groupCount; ++i) {
        QModelIndex groupIndex = groupModel.index(i, 0);
        ASSERT_TRUE(groupIndex.isValid());

        int childCount = groupModel.rowCount(groupIndex);
        EXPECT_GT(childCount, 0);

        // 测试正常子索引
        for (int j = 0; j < childCount; ++j) {
            QModelIndex childIndex = groupModel.index(j, 0, groupIndex);
            EXPECT_TRUE(childIndex.isValid());

            // 测试数据访问
            QVariant data = childIndex.data(Qt::DisplayRole);
            EXPECT_TRUE(data.isValid());
        }

        // 测试越界子索引（模拟滚动越界的情况）
        for (int j = childCount; j < childCount + 10; ++j) {
            QModelIndex invalidChild = groupModel.index(j, 0, groupIndex);
            EXPECT_FALSE(invalidChild.isValid());
        }
    }

    // 测试大量越界的顶级索引
    for (int i = groupCount; i < groupCount + 100; ++i) {
        QModelIndex invalidIndex = groupModel.index(i, 0);
        EXPECT_FALSE(invalidIndex.isValid());
    }

    // 测试负数索引
    QModelIndex negativeIndex = groupModel.index(-1, 0);
    EXPECT_FALSE(negativeIndex.isValid());

    QModelIndex firstGroup = groupModel.index(0, 0);
    if (firstGroup.isValid()) {
        QModelIndex negativeChild = groupModel.index(-1, 0, firstGroup);
        EXPECT_FALSE(negativeChild.isValid());
    }
}
