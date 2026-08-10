// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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

// 测试析构函数是否正确释放内存（内存泄漏检测）
TEST(RoleGroupModel, DestructorMemoryLeakTest)
{
    // 这个测试验证当 RoleGroupModel 被销毁时，所有内部分配的 QList 对象都被正确释放
    
    auto role = Qt::UserRole + 1;
    
    // 创建源模型并添加数据
    QStandardItemModel *sourceModel = new QStandardItemModel();
    for (int i = 0; i < 5; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group%1").arg(i % 2), role);
        sourceModel->appendRow(item);
    }
    
    // 创建 RoleGroupModel 并验证数据
    RoleGroupModel *groupModel = new RoleGroupModel(sourceModel, role);
    EXPECT_EQ(groupModel->rowCount(), 2); // 应该有2个分组
    
    // 销毁 RoleGroupModel - 应该释放所有内部 QList 对象
    EXPECT_NO_THROW({
        delete groupModel;
        groupModel = nullptr;
    });
    
    // 销毁源模型
    delete sourceModel;
    
    // 如果存在内存泄漏，ASan 会在这里报告
    EXPECT_EQ(groupModel, nullptr);
}

// 测试多次重建数据源时的内存管理
TEST(RoleGroupModel, RebuildTreeSourceMemoryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);
    
    // 第一次添加数据
    for (int i = 0; i < 3; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group1"), role);
        model.appendRow(item);
    }
    EXPECT_EQ(groupModel.rowCount(), 1);
    
    // 修改数据触发重建（通过 dataChanged 信号）
    for (int i = 0; i < model.rowCount(); ++i) {
        model.setData(model.index(i, 0), QString("group%1").arg(i), role);
    }
    
    // 重建后应该有3个分组
    EXPECT_EQ(groupModel.rowCount(), 3);
    
    // 再次修改数据触发重建
    for (int i = 0; i < model.rowCount(); ++i) {
        model.setData(model.index(i, 0), QString("newgroup"), role);
    }
    
    // 重建后应该只有1个分组
    EXPECT_EQ(groupModel.rowCount(), 1);

}

// 测试设置不同的 sourceModel 时的内存管理
TEST(RoleGroupModel, SetSourceModelMemoryTest)
{
    auto role = Qt::UserRole + 1;
    
    // 创建第一个源模型
    QStandardItemModel model1;
    for (int i = 0; i < 3; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group1"), role);
        model1.appendRow(item);
    }
    
    // 创建 RoleGroupModel
    RoleGroupModel groupModel(&model1, role);
    EXPECT_EQ(groupModel.rowCount(), 1);
    
    // 创建第二个源模型
    QStandardItemModel model2;
    for (int i = 0; i < 5; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group%1").arg(i), role);
        model2.appendRow(item);
    }
    
    // 切换到新的源模型
    groupModel.setSourceModel(&model2);
    EXPECT_EQ(groupModel.rowCount(), 5);
    
    // 设置空源模型
    groupModel.setSourceModel(nullptr);
    EXPECT_EQ(groupModel.rowCount(), 0);
    
    // 再次设置源模型
    groupModel.setSourceModel(&model1);
    EXPECT_EQ(groupModel.rowCount(), 1);
    
}

// 测试空模型和边界情况的内存管理
TEST(RoleGroupModel, EmptyModelMemoryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    
    // 创建空的 RoleGroupModel
    RoleGroupModel groupModel(&model, role);
    EXPECT_EQ(groupModel.rowCount(), 0);
    
    // 添加一些数据
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("group1"), role);
    model.appendRow(item1);
    EXPECT_EQ(groupModel.rowCount(), 1);
    
    // 清空模型
    model.clear();
    EXPECT_EQ(groupModel.rowCount(), 0);
    
    // 再次添加数据
    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("group2"), role);
    model.appendRow(item2);
    EXPECT_EQ(groupModel.rowCount(), 1);
    
    // 再次清空
    model.clear();
    EXPECT_EQ(groupModel.rowCount(), 0);
    
}

// 测试大量数据操作的内存稳定性
TEST(RoleGroupModel, LargeDataMemoryStabilityTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);
    
    // 添加大量数据
    const int itemCount = 100;
    for (int i = 0; i < itemCount; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group%1").arg(i % 10), role);
        model.appendRow(item);
    }
    
    EXPECT_EQ(groupModel.rowCount(), 10);
    
    // 删除一半数据
    model.removeRows(0, itemCount / 2);
    EXPECT_EQ(groupModel.rowCount(), 10); // 分组可能还在，但子项减少
    
    // 再次添加数据
    for (int i = 0; i < itemCount / 2; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("newgroup%1").arg(i % 5), role);
        model.appendRow(item);
    }
    
}

// 测试 setDeduplicationRole 改变时的内存管理
TEST(RoleGroupModel, SetDeduplicationRoleMemoryTest)
{
    QStandardItemModel model;
    auto role1 = Qt::UserRole + 1;
    auto role2 = Qt::UserRole + 2;
    
    // 设置两个角色的数据
    for (int i = 0; i < 5; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("groupA"), role1);
        item->setData(QString("group%1").arg(i % 3), role2);
        model.appendRow(item);
    }
    
    RoleGroupModel groupModel(&model, role1);
    EXPECT_EQ(groupModel.rowCount(), 1); // role1: 1个分组
    
    // 切换到 role2
    groupModel.setDeduplicationRole(role2);
    EXPECT_EQ(groupModel.rowCount(), 3); // role2: 3个分组
    
    // 切换回 role1
    groupModel.setDeduplicationRole(role1);
    EXPECT_EQ(groupModel.rowCount(), 1); // role1: 1个分组
    
}

// 测试 rowsInserted 信号处理时的内存管理
TEST(RoleGroupModel, RowsInsertedMemoryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);
    
    // 初始添加数据
    for (int i = 0; i < 3; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group1"), role);
        model.appendRow(item);
    }
    EXPECT_EQ(groupModel.rowCount(), 1);
    EXPECT_EQ(groupModel.rowCount(groupModel.index(0, 0)), 3);
    
    // 插入新行到现有分组
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("group1"), role);
    model.insertRow(1, item1);
    EXPECT_EQ(groupModel.rowCount(groupModel.index(0, 0)), 4);
    
    // 插入新行创建新分组
    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("group2"), role);
    model.appendRow(item2);
    EXPECT_EQ(groupModel.rowCount(), 2);
    
}

// 测试 rowsRemoved 信号处理时的内存管理（包括删除空分组）
TEST(RoleGroupModel, RowsRemovedWithEmptyGroupMemoryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);
    
    // 添加数据到分组
    QStandardItem *item1 = new QStandardItem;
    item1->setData(QString("group1"), role);
    model.appendRow(item1);
    
    QStandardItem *item2 = new QStandardItem;
    item2->setData(QString("group1"), role);
    model.appendRow(item2);
    
    QStandardItem *item3 = new QStandardItem;
    item3->setData(QString("group2"), role);
    model.appendRow(item3);
    
    EXPECT_EQ(groupModel.rowCount(), 2);
    
    // 删除 group1 的所有项目，触发分组删除
    model.removeRow(0);
    model.removeRow(0); // 注意：删除第一行后，原来的第二行变成了第一行
    
    // 现在应该只有 group2
    EXPECT_EQ(groupModel.rowCount(), 1);
    
    // 删除最后一个项目
    model.removeRow(0);
    EXPECT_EQ(groupModel.rowCount(), 0);
    
}


// 测试 modelReset 信号处理时的内存管理
TEST(RoleGroupModel, ModelResetMemoryTest)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);
    
    // 添加数据
    for (int i = 0; i < 5; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("group%1").arg(i % 2), role);
        model.appendRow(item);
    }
    EXPECT_EQ(groupModel.rowCount(), 2);
    
    // 使用 clear() 方法会触发 modelReset 信号
    model.clear();
    
    // 重新添加不同的数据
    for (int i = 0; i < 3; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(QString("newgroup"), role);
        model.appendRow(item);
    }
    
    EXPECT_EQ(groupModel.rowCount(), 1);
    
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

// ---- 验证 Bug: RoleGroupModel::rowsRemoved 批量删除时跳过后续分组 ----
// 当一次 rowsRemoved 覆盖多行、且中间某个分组被删空时，该分组从 m_rowMap 移除后
// 循环下标前移，导致后续分组被跳过、其成员行未被移除，
// 随后 adjustMap 甚至可能把残留的源行号修正为负数。
TEST(RoleGroupModel, RowsRemovedRangeSkip)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 构造 3 个分组：g0=[0], g1=[2], g2=[3,4]（行 1 为空数据, 不分组）
    QStandardItem *g0a = new QStandardItem;
    g0a->setData(QString("g0"), role);
    model.appendRow(g0a);                           // source row 0

    model.appendRow(new QStandardItem);             // source row 1, empty -> not grouped

    QStandardItem *g1a = new QStandardItem;
    g1a->setData(QString("g1"), role);
    model.appendRow(g1a);                           // source row 2

    QStandardItem *g2a = new QStandardItem;
    g2a->setData(QString("g2"), role);
    model.appendRow(g2a);                           // source row 3

    QStandardItem *g2b = new QStandardItem;
    g2b->setData(QString("g2"), role);
    model.appendRow(g2b);                           // source row 4

    ASSERT_EQ(groupModel.rowCount(), 3);            // g0, g1, g2
    ASSERT_EQ(groupModel.rowCount(groupModel.index(0, 0)), 1);  // g0: 1 child
    ASSERT_EQ(groupModel.rowCount(groupModel.index(1, 0)), 1);  // g1: 1 child
    ASSERT_EQ(groupModel.rowCount(groupModel.index(2, 0)), 2);  // g2: 2 children

    // 删除 source rows 0..2：g0 的唯一子项被删（g0 变空），g1 的唯一子项被删
    // BUG: g0 变空后从 m_rowMap 移除，循环下标前移，g2 中的 row 3 不会被处理
    // 而且 g2 中的 row 2 残留，adjustMap 后变成负数。
    model.removeRows(0, 3);

    // g2 应保留，有 2 个子项（原 row 3,4 调整后变为 row 0,1）
    EXPECT_EQ(groupModel.rowCount(), 1) << "FAIL: expected 1 group (g2), got " << groupModel.rowCount();

    if (groupModel.rowCount() > 0) {
        auto g2Idx = groupModel.index(0, 0);
        int childCount = groupModel.rowCount(g2Idx);
        EXPECT_EQ(childCount, 2) << "FAIL: g2 should have 2 children, got " << childCount;

        // 验证 mapToSource 返回的索引都在有效范围内
        for (int i = 0; i < childCount; ++i) {
            auto child = groupModel.index(i, 0, g2Idx);
            auto src = groupModel.mapToSource(child);
            EXPECT_TRUE(src.isValid()) << "FAIL: child " << i << " maps to invalid source row";
            EXPECT_LT(src.row(), model.rowCount()) << "FAIL: child " << i << " source row out of range";
        }
    }
}

// ---- 验证 Bug: RoleGroupModel 发出带有效 parent 的子级 dataChanged ----
// 当源模型中分组内某行的非去重角色数据改变时，RoleGroupModel 发出的 dataChanged
// 是一个子级索引（parent 有效）。下游的 DockItemModel 在分组模式下直接取
// topLeft.row() 作为顶层行号，导致刷新到错误的行或越界。
TEST(RoleGroupModel, ChildDataChangedHasValidParent)
{
    QStandardItemModel model;
    auto role = Qt::UserRole + 1;
    RoleGroupModel groupModel(&model, role);

    // 构造一个分组 "app" 包含 2 个子项
    QStandardItem *a = new QStandardItem;
    a->setData(QString("app"), role);
    model.appendRow(a);

    QStandardItem *b = new QStandardItem;
    b->setData(QString("app"), role);
    model.appendRow(b);

    ASSERT_EQ(groupModel.rowCount(), 1);
    auto groupIdx = groupModel.index(0, 0);
    ASSERT_EQ(groupModel.rowCount(groupIdx), 2);

    // 监听 dataChanged 信号，检查 child 索引的 parent 是否有效
    QSignalSpy spy(&groupModel, &QAbstractItemModel::dataChanged);

    // 改变非去重角色（Qt::DisplayRole），不触发去重重建
    model.setData(model.index(1, 0), QVariant("new-title"), Qt::DisplayRole);

    bool sawChildWithValidParent = false;
    for (const auto &args : spy) {
        if (args.size() >= 1) {
            QModelIndex tl = args[0].value<QModelIndex>();
            if (tl.parent().isValid()) {
                sawChildWithValidParent = true;
                break;
            }
        }
    }

    // BUG: 子级 dataChanged 会被下游 DockItemModel 错误地转发为顶层行号
    EXPECT_TRUE(sawChildWithValidParent)
        << "FAIL: RoleGroupModel should emit child dataChanged with valid parent";
}
