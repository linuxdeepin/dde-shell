// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QAbstractItemModelTester>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QSignalSpy>
#include <QVariant>

#include <memory>

#include "appletitemmodel.h"

using namespace ds;

// An empty model reports zero rows and exposes the single "data" role.
TEST(DAppletItemModel, EmptyModel)
{
    DAppletItemModel model;
    EXPECT_EQ(model.rowCount(QModelIndex()), 0);
    EXPECT_EQ(model.rowCount(QModelIndex()), 0);
    EXPECT_TRUE(model.rootObjects().isEmpty());

    const auto roleNames = model.roleNames();
    ASSERT_TRUE(roleNames.contains(DAppletItemModel::Data));
    EXPECT_EQ(roleNames.value(DAppletItemModel::Data), QByteArrayLiteral("data"));
}

// append() grows rowCount and emits rowsInserted with the right span.
TEST(DAppletItemModel, AppendGrowsRowCount)
{
    DAppletItemModel model;
    QObject o1, o2;

    QSignalSpy insertSpy(&model, &DAppletItemModel::rowsInserted);
    ASSERT_TRUE(insertSpy.isValid());

    model.append(&o1);
    ASSERT_EQ(insertSpy.count(), 1);
    const auto args1 = insertSpy.takeFirst();
    EXPECT_EQ(args1.at(1).toInt(), 0); // first
    EXPECT_EQ(args1.at(2).toInt(), 0); // last
    EXPECT_EQ(model.rowCount(QModelIndex()), 1);

    model.append(&o2);
    ASSERT_EQ(insertSpy.count(), 1);
    const auto args2 = insertSpy.takeFirst();
    EXPECT_EQ(args2.at(1).toInt(), 1); // first
    EXPECT_EQ(args2.at(2).toInt(), 1); // last
    EXPECT_EQ(model.rowCount(QModelIndex()), 2);

    EXPECT_EQ(model.rootObjects().size(), 2);
    EXPECT_EQ(model.rootObjects().first(), &o1);
    EXPECT_EQ(model.rootObjects().last(), &o2);
}

// data() returns the stored QObject* under the Data role.
TEST(DAppletItemModel, DataReturnsStoredObject)
{
    DAppletItemModel model;
    QObject o1, o2;
    model.append(&o1);
    model.append(&o2);

    EXPECT_EQ(model.index(0).data(DAppletItemModel::Data).value<QObject *>(), &o1);
    EXPECT_EQ(model.index(1).data(DAppletItemModel::Data).value<QObject *>(), &o2);
}

// data() for an out-of-range row returns an empty QVariant (observable contract).
TEST(DAppletItemModel, DataOutOfRangeIsEmpty)
{
    DAppletItemModel model;
    QObject o;
    model.append(&o);

    const QModelIndex outOfRange = model.index(model.rowCount(QModelIndex()), 0);
    EXPECT_FALSE(outOfRange.isValid());
    EXPECT_FALSE(outOfRange.data(DAppletItemModel::Data).isValid());
}

// data() with an unknown role returns an empty QVariant.
TEST(DAppletItemModel, DataUnknownRole)
{
    DAppletItemModel model;
    QObject o;
    model.append(&o);
    EXPECT_FALSE(model.index(0).data(Qt::UserRole + 999).isValid());
}

// remove() shrinks rowCount and emits rowsRemoved with the right span.
TEST(DAppletItemModel, RemoveShrinksRowCount)
{
    DAppletItemModel model;
    QObject o1, o2, o3;
    model.append(&o1);
    model.append(&o2);
    model.append(&o3);
    ASSERT_EQ(model.rowCount(QModelIndex()), 3);

    QSignalSpy removeSpy(&model, &DAppletItemModel::rowsRemoved);
    ASSERT_TRUE(removeSpy.isValid());

    model.remove(&o2);
    ASSERT_EQ(removeSpy.count(), 1);
    const auto args = removeSpy.takeFirst();
    EXPECT_EQ(args.at(1).toInt(), 1); // first
    EXPECT_EQ(args.at(2).toInt(), 1); // last
    EXPECT_EQ(model.rowCount(QModelIndex()), 2);

    const auto roots = model.rootObjects();
    ASSERT_EQ(roots.size(), 2);
    EXPECT_EQ(roots.at(0), &o1);
    EXPECT_EQ(roots.at(1), &o3);
}

// remove() of an object not present is a no-op (no signal, no row change).
TEST(DAppletItemModel, RemoveMissingIsNoOp)
{
    DAppletItemModel model;
    QObject o1, other;
    model.append(&o1);
    ASSERT_EQ(model.rowCount(QModelIndex()), 1);

    QSignalSpy removeSpy(&model, &DAppletItemModel::rowsRemoved);
    model.remove(&other);
    EXPECT_EQ(removeSpy.count(), 0);
    EXPECT_EQ(model.rowCount(QModelIndex()), 1);
    EXPECT_EQ(model.rootObjects().size(), 1);
}

// Removing down to empty leaves rowCount 0 and an empty object list.
TEST(DAppletItemModel, RemoveUntilEmpty)
{
    DAppletItemModel model;
    QObject o1, o2;
    model.append(&o1);
    model.append(&o2);
    model.remove(&o1);
    model.remove(&o2);
    EXPECT_EQ(model.rowCount(QModelIndex()), 0);
    EXPECT_TRUE(model.rootObjects().isEmpty());
}

// rootObjects() reflects the live internal list after mutations.
TEST(DAppletItemModel, RootObjectsReflectsMutations)
{
    DAppletItemModel model;
    QObject o1, o2;
    model.append(&o1);
    model.append(&o2);

    ASSERT_EQ(model.rootObjects().size(), 2);
    EXPECT_EQ(model.rootObjects().at(0), &o1);
    EXPECT_EQ(model.rootObjects().at(1), &o2);

    model.remove(&o1);
    ASSERT_EQ(model.rootObjects().size(), 1);
    EXPECT_EQ(model.rootObjects().at(0), &o2);
}

// QAbstractItemModelTester validates model invariants across mutations.
// If the model violates Qt model/view contracts, the tester asserts fatally.
TEST(DAppletItemModel, ModelTesterValidation)
{
    DAppletItemModel model;
    auto tester = std::make_unique<QAbstractItemModelTester>(
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    QObject o1, o2, o3, o4;
    model.append(&o1);
    model.append(&o2);
    model.append(&o3);
    model.remove(&o2);
    model.append(&o4);
    model.remove(&o1);
    model.remove(&o3);
    model.remove(&o4);

    EXPECT_EQ(model.rowCount(QModelIndex()), 0);
}

// ModelTester on an initially-empty model with append/remove interleaving.
TEST(DAppletItemModel, ModelTesterAppendRemoveInterleaved)
{
    DAppletItemModel model;
    auto tester = std::make_unique<QAbstractItemModelTester>(
        &model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    QObject items[4];
    model.append(&items[0]);
    model.append(&items[1]);
    model.remove(&items[0]);
    model.append(&items[2]);
    model.remove(&items[1]);
    model.append(&items[3]);
    model.remove(&items[2]);
    model.remove(&items[3]);
    EXPECT_EQ(model.rowCount(QModelIndex()), 0);
}
