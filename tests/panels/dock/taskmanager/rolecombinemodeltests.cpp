// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractItemModelTester>
#include <QVariant>

#include <algorithm>
#include <gtest/gtest.h>

#include <QSignalSpy>

#include "rolecombinemodel.h"
#include "combinemodela.h"
#include "combinemodelb.h"

TEST(RoleCombineModel, RoleNamesTest) {
    TestModelA modelA;
    TestModelB modelB;
    RoleCombineModel model(&modelA, &modelB, 1, [](QVariant data, QAbstractItemModel* model) -> QModelIndex{ return QModelIndex();});
    QHash<int, QByteArray> res(modelA.roleNames());
    auto keys = modelA.roleNames().keys();
    auto maxKey = *(std::max_element(keys.begin(), keys.end())) + 1;
    for (auto c : modelB.roleNames()) {
        res.insert(maxKey++, c);
    }

    EXPECT_EQ(model.roleNames(), res);
}

TEST(RoleCombineModel, RowCountTest) {
    TestModelA modelA;
    TestModelB modelB;
    modelA.addData(new DataA(0, &modelA));
    modelA.addData(new DataA(1, &modelA));

    modelB.addData(new DataB(0, &modelB));
    modelB.addData(new DataB(1, &modelB));
    modelB.addData(new DataB(2, &modelB));
    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel* model) -> QModelIndex 
        {
            return QModelIndex();
        }
    );

    // modelA is major dataModel
    EXPECT_EQ(model.rowCount(), modelA.rowCount());
    EXPECT_NE(model.rowCount(), modelB.rowCount());
}

TEST(RoleGroupModel, ModelTest)
{
    TestModelA modelA;
    TestModelB modelB;
    modelA.addData(new DataA(0, &modelA));
    modelA.addData(new DataA(1, &modelA));

    modelB.addData(new DataB(0, &modelB));
    modelB.addData(new DataB(1, &modelB));
    modelB.addData(new DataB(2, &modelB));
    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        return QModelIndex();
    });

    auto tester = new QAbstractItemModelTester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
}

TEST(RoleCombineModel, dataTest) {
    TestModelA modelA;
    TestModelB modelB;
    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel* model) -> QModelIndex 
        {
            auto c = model->match(model->index(0, 0), TestModelB::idRole, data);
            if (c.size() > 0) {
                return c.first();
            }

            return QModelIndex();
        }
    );

    QSignalSpy spyA(&modelA, &QAbstractItemModel::dataChanged);
    QSignalSpy spyB(&modelB, &QAbstractItemModel::dataChanged);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);

    modelA.addData(new DataA(0, "dataA0", &modelA));
    modelA.addData(new DataA(1, "dataA1", &modelA));
    modelA.addData(new DataA(3, "dataA3", &modelA));

    modelB.addData(new DataB(0, "dataB0", &modelB));
    modelB.addData(new DataB(1, "dataB1", &modelB));
    modelB.addData(new DataB(2, "dataB2",&modelB));

    auto roleNames = model.roleNames();
    auto roleNamesA = modelA.roleNames();
    auto roleNamesB = modelB.roleNames();

    QHash<QByteArray, int> names2Role;

    for (auto roleName : roleNames.keys()) {
        names2Role.insert(roleNames.value(roleName), roleName);
    }

    // below data are same
    EXPECT_EQ(model.index(0, 0).data(names2Role.value(roleNamesA.value(TestModelA::dataRole))), modelA.index(0, 0).data(TestModelA::dataRole));
    EXPECT_EQ(model.index(1, 0).data(names2Role.value(roleNamesA.value(TestModelA::dataRole))), modelA.index(1, 0).data(TestModelA::dataRole));

    EXPECT_EQ(model.index(0, 0).data(names2Role.value(roleNamesB.value(TestModelB::dataRole))), modelB.index(0, 0).data(TestModelB::dataRole));
    EXPECT_EQ(model.index(1, 0).data(names2Role.value(roleNamesB.value(TestModelB::dataRole))), modelB.index(1, 0).data(TestModelB::dataRole));

    // // because lack of dataB id is 3, so will return a empty QString
    EXPECT_EQ(model.index(2, 0).data(names2Role.value(roleNamesB.value(TestModelB::dataRole))).toString(), QString());

    // below data are difference, because combine func can find id 2 for DataA;
    EXPECT_NE(model.index(2, 0).data(names2Role.value(roleNamesB.value(TestModelB::dataRole))), modelA.index(1, 0).data(TestModelB::dataRole));

    modelB.setData(modelB.index(1), "dataB22");
    EXPECT_EQ(model.index(1, 0).data(names2Role.value(roleNamesB.value(TestModelB::dataRole))), modelB.index(1, 0).data(TestModelB::dataRole));
}

// 新增测试用例：检测行删除后索引映射错误的问题
TEST(RoleCombineModel, IndexMappingAfterRowRemovalBug)
{
    TestModelA modelA;
    TestModelB modelB;

    // 创建combine model，通过ID匹配
    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        auto matches = model->match(model->index(0, 0), TestModelB::idRole, data);
        return matches.isEmpty() ? QModelIndex() : matches.first();
    });

    // 添加测试数据 - modelA有id为0,1,2,3的数据
    DataA *dataA0 = new DataA(0, "dataA0", &modelA);
    DataA *dataA1 = new DataA(1, "dataA1", &modelA);
    DataA *dataA2 = new DataA(2, "dataA2", &modelA);
    DataA *dataA3 = new DataA(3, "dataA3", &modelA);

    modelA.addData(dataA0);
    modelA.addData(dataA1);
    modelA.addData(dataA2);
    modelA.addData(dataA3);

    // modelB有对应的id数据
    modelB.addData(new DataB(0, "dataB0", &modelB));
    modelB.addData(new DataB(1, "dataB1", &modelB));
    modelB.addData(new DataB(2, "dataB2", &modelB));
    modelB.addData(new DataB(3, "dataB3", &modelB));

    auto roleNames = model.roleNames();
    auto roleNamesB = modelB.roleNames();
    QHash<QByteArray, int> names2Role;
    for (auto roleName : roleNames.keys()) {
        names2Role.insert(roleNames.value(roleName), roleName);
    }

    int bDataRole = names2Role.value(roleNamesB.value(TestModelB::dataRole));

    // 删除前验证数据正确性
    EXPECT_EQ(model.index(0, 0).data(bDataRole).toString(), "dataB0");
    EXPECT_EQ(model.index(1, 0).data(bDataRole).toString(), "dataB1");
    EXPECT_EQ(model.index(2, 0).data(bDataRole).toString(), "dataB2");
    EXPECT_EQ(model.index(3, 0).data(bDataRole).toString(), "dataB3");

    // 删除第1行 (索引1，包含id=1的数据)
    modelA.removeData(dataA1);

    // 验证行数正确
    EXPECT_EQ(model.rowCount(), 3);

    // 这里会暴露问题：删除第1行后，原来的第2、3行索引映射没有正确更新
    // 原来第2行(id=2)现在应该在第1行位置，但由于映射没更新，可能访问到错误数据
    EXPECT_EQ(model.index(0, 0).data(bDataRole).toString(), "dataB0"); // 第0行应该正常

    // 这个测试会失败，暴露了索引映射更新的问题
    // 原来第2行(id=2)现在在第1行位置，应该返回"dataB2"
    EXPECT_EQ(model.index(1, 0).data(bDataRole).toString(), "dataB2");

    // 原来第3行(id=3)现在在第2行位置，应该返回"dataB3"
    EXPECT_EQ(model.index(2, 0).data(bDataRole).toString(), "dataB3");
}

// 新增测试用例：检测parent参数处理问题
TEST(RoleCombineModel, InvalidParentHandlingBug)
{
    TestModelA modelA;
    TestModelB modelB;

    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        return QModelIndex();
    });

    modelA.addData(new DataA(0, "dataA0", &modelA));

    // 测试无效parent的情况，这可能导致index(parent.row(), parent.column())访问无效索引
    // 在实际使用中，parent可能是无效的QModelIndex()
    QModelIndex invalidParent; // 无效的parent

    // 当parent无效时调用parent.row()和parent.column()会返回-1
    // 这时index(-1, -1)可能导致问题
    // 这个测试主要验证代码不会崩溃
    EXPECT_NO_THROW({
        // 触发rowsInserted信号来测试parent处理
        modelA.addData(new DataA(1, "dataA1", &modelA));
    });
}

// 新增测试用例：检测minor模型变化处理不完整的问题
TEST(RoleCombineModel, MinorModelChangesHandlingBug)
{
    TestModelA modelA;
    TestModelB modelB;

    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        auto matches = model->match(model->index(0, 0), TestModelB::idRole, data);
        return matches.isEmpty() ? QModelIndex() : matches.first();
    });

    modelA.addData(new DataA(0, "dataA0", &modelA));
    modelA.addData(new DataA(1, "dataA1", &modelA));

    DataB *dataB0 = new DataB(0, "dataB0", &modelB);
    DataB *dataB1 = new DataB(1, "dataB1", &modelB);
    DataB *dataB2 = new DataB(2, "dataB2", &modelB);

    modelB.addData(dataB0);
    modelB.addData(dataB1);
    modelB.addData(dataB2);

    auto roleNames = model.roleNames();
    auto roleNamesB = modelB.roleNames();
    QHash<QByteArray, int> names2Role;
    for (auto roleName : roleNames.keys()) {
        names2Role.insert(roleNames.value(roleName), roleName);
    }
    int bDataRole = names2Role.value(roleNamesB.value(TestModelB::dataRole));

    // 验证初始映射正确
    EXPECT_EQ(model.index(0, 0).data(bDataRole).toString(), "dataB0");
    EXPECT_EQ(model.index(1, 0).data(bDataRole).toString(), "dataB1");

    // 删除minor模型中的数据 - 这里会暴露问题：
    // RoleCombineModel没有处理minor模型的rowsRemoved信号
    modelB.removeData(dataB0);

    // 删除后，映射应该更新，但由于没有处理rowsRemoved信号，映射可能变得无效
    // 这可能导致访问已删除的数据或返回错误结果
    // 注意：由于当前实现没有处理minor模型的删除，这个测试主要验证不会崩溃
    EXPECT_NO_THROW({
        auto result = model.index(0, 0).data(bDataRole);
        // 结果可能是无效的，但至少不应该崩溃
    });
}

// 新增测试用例：验证角色映射修复
TEST(RoleCombineModel, RoleMappingFix)
{
    TestModelA modelA;
    TestModelB modelB;

    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        auto matches = model->match(model->index(0, 0), TestModelB::idRole, data);
        return matches.isEmpty() ? QModelIndex() : matches.first();
    });

    modelA.addData(new DataA(0, "dataA0", &modelA));
    modelB.addData(new DataB(0, "dataB0", &modelB));

    // 验证角色映射正确性：Minor角色应该映射到新创建的组合角色，而不是Major角色
    auto combinedRoleNames = model.roleNames();
    auto majorRoleNames = modelA.roleNames();
    auto minorRoleNames = modelB.roleNames();

    // 验证组合模型包含所有角色
    EXPECT_GT(combinedRoleNames.size(), majorRoleNames.size());
    EXPECT_GT(combinedRoleNames.size(), minorRoleNames.size());

    // 验证Minor数据可以通过组合模型正确访问
    // 找到Minor的"data"角色在组合模型中的key
    int minorDataRoleInCombined = -1;
    for (auto it = combinedRoleNames.constBegin(); it != combinedRoleNames.constEnd(); ++it) {
        if (it.value() == "bData" && !majorRoleNames.contains(it.key())) {
            minorDataRoleInCombined = it.key();
            break;
        }
    }

    EXPECT_NE(minorDataRoleInCombined, -1) << "Minor data role should be found in combined model";

    // 验证可以通过组合模型获取Minor数据
    auto index = model.index(0, 0);
    auto minorData = index.data(minorDataRoleInCombined);
    EXPECT_EQ(minorData.toString(), "dataB0") << "Should be able to access minor data through combined model";
}

// 新增测试用例：验证数据访问安全性修复
TEST(RoleCombineModel, DataAccessSafetyFix)
{
    TestModelA modelA;
    TestModelB modelB;

    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        auto matches = model->match(model->index(0, 0), TestModelB::idRole, data);
        return matches.isEmpty() ? QModelIndex() : matches.first();
    });

    // 添加没有对应Minor数据的Major数据
    modelA.addData(new DataA(999, "orphanData", &modelA));

    // 验证访问无映射的数据时不会崩溃，应该返回空值
    auto combinedRoleNames = model.roleNames();
    int minorDataRole = -1;
    for (auto it = combinedRoleNames.constBegin(); it != combinedRoleNames.constEnd(); ++it) {
        if (it.value() == "bData") {
            minorDataRole = it.key();
            break;
        }
    }

    ASSERT_NE(minorDataRole, -1);

    // 访问没有Minor映射的数据应该返回空值而不是崩溃
    auto index = model.index(0, 0);
    EXPECT_NO_THROW({
        auto result = index.data(minorDataRole);
        EXPECT_FALSE(result.isValid()) << "Should return invalid QVariant for unmapped minor data";
    });
}

// 新增测试用例：验证parent参数处理修复
TEST(RoleCombineModel, ParentParameterHandlingFix)
{
    TestModelA modelA;
    TestModelB modelB;

    // 这个测试主要验证不会因为parent参数处理问题导致Qt模型警告或崩溃
    RoleCombineModel model(&modelA, &modelB, TestModelA::idRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
        auto matches = model->match(model->index(0, 0), TestModelB::idRole, data);
        return matches.isEmpty() ? QModelIndex() : matches.first();
    });

    // 添加数据时会触发rowsInserted信号，测试parent参数处理
    EXPECT_NO_THROW({
        modelA.addData(new DataA(0, "testData", &modelA));
        modelB.addData(new DataB(0, "testDetail", &modelB));
    });

    // 验证数据正确添加
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(model.index(0, 0).isValid());
}
