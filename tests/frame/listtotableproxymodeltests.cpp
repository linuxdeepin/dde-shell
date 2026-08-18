// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for ListToTableProxyModel (frame/models/listtotableproxymodel),
// the concrete KExtraColumnsProxyModel subclass that turns a list model's roles
// into table columns.
//
// NOTE: private-member access (m_roles / m_sourceColumn) uses a scoped
// `#define private public` block around the model header below, NOT a global
// -D compile definition (the latter breaks gtest/libstdc++). In production
// `roles` is only ever set from QML (TrayContainer.qml), and QList<int> has
// no Q_DECLARE_METATYPE in this repo, so a C++ setProperty("roles", ...) is
// unreliable; direct member access is the robust white-box alternative and
// avoids the m_roles[extraColumn] out-of-bounds UB when m_roles is empty.
// If upstream ListToTableProxyModel::m_roles changes (type/semantics), the
// direct-write sites here must be updated in sync.

#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QList>
#include <QModelIndex>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QVariant>

// Scoped visibility: expose private members of ListToTableProxyModel ONLY
// while including its header, then restore access. This avoids the global
// -Dprivate=public that breaks gtest/libstdc++ internal headers.
#define private public
#define protected public
#include "listtotableproxymodel.h"
#undef private
#undef protected

namespace {
constexpr int kNameRole = Qt::UserRole + 1;
constexpr int kValueRole = Qt::UserRole + 2;
constexpr int kListRole = Qt::UserRole + 3;
} // namespace

// Helper: create a 1-column source model with the given role names, no rows.
QStandardItemModel *makeEmptySourceWithRoles(QObject *parent)
{
    auto *src = new QStandardItemModel(parent);
    src->setItemRoleNames({{kNameRole, "name"},
                           {kValueRole, "value"},
                           {kListRole, "list"}});
    return src;
}

// Setting m_roles and emitting rolesChanged drives the appendColumn() lambda:
// one extra column is appended per role, named after the source role name.
TEST(ListToTableProxyModel, RolesChangedAppendsColumns)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}, {kValueRole, "value"}});
    src.appendRow(new QStandardItem);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);

    EXPECT_EQ(proxy.columnCount(), 1); // only the source column, no extras yet

    // White-box: populate m_roles and fire rolesChanged (the production path is
    // QML setting the `roles` property, which triggers the same signal).
    proxy.m_roles = QList<int>{kNameRole, kValueRole};
    proxy.rolesChanged(proxy.m_roles);

    // Two extra columns appended -> columnCount = 1 (source) + 2.
    EXPECT_EQ(proxy.columnCount(), 1 + 2);
    // Extra columns are titled after the source role names.
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("name"));
    EXPECT_EQ(proxy.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("value"));
}

// extraColumnData returns the source value at (row, sourceColumn) for the
// role stored in m_roles[extraColumn] — the normal "valid value" return path.
TEST(ListToTableProxyModel, ExtraColumnDataReturnsSourceValue)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}, {kValueRole, "value"}});
    QStandardItem *item = new QStandardItem;
    item->setData(QStringLiteral("n0"), kNameRole);
    item->setData(QStringLiteral("v0"), kValueRole);
    src.appendRow(item);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kNameRole, kValueRole};
    proxy.rolesChanged(proxy.m_roles);

    // Extra column 0 (proxy col 1) -> m_roles[0] = name role -> "n0".
    EXPECT_EQ(proxy.data(proxy.index(0, 1), Qt::DisplayRole).toString(),
              QStringLiteral("n0"));
    // Extra column 1 (proxy col 2) -> m_roles[1] = value role -> "v0".
    EXPECT_EQ(proxy.data(proxy.index(0, 2), Qt::DisplayRole).toString(),
              QStringLiteral("v0"));
}

// extraColumnData returns the "<invalid>" placeholder when the source has no
// data for the requested role (the `!result.isValid()` branch).
TEST(ListToTableProxyModel, ExtraColumnDataInvalidReturnsPlaceholder)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}, {kValueRole, "value"}});
    QStandardItem *item = new QStandardItem;
    item->setData(QStringLiteral("n0"), kNameRole);
    // NOTE: kValueRole is intentionally NOT set -> data() returns an invalid
    // QVariant, exercising the "<invalid>" placeholder return.
    src.appendRow(item);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kNameRole, kValueRole};
    proxy.rolesChanged(proxy.m_roles);

    EXPECT_EQ(proxy.data(proxy.index(0, 2), Qt::DisplayRole).toString(),
              QStringLiteral("<invalid>"));
}

// extraColumnData joins a QVariantList result with ',' (the
// `result.userType() == QMetaType::QVariantList` branch). NOTE: must store a
// QVariantList (userType==9), not a QStringList (userType==11) — the latter
// does NOT match the branch and returns an empty string.
TEST(ListToTableProxyModel, ExtraColumnDataVariantListJoined)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kListRole, "list"}});
    QStandardItem *item = new QStandardItem;
    item->setData(QVariantList{QStringLiteral("a"), QStringLiteral("b")}, kListRole);
    src.appendRow(item);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kListRole};
    proxy.rolesChanged(proxy.m_roles);

    EXPECT_EQ(proxy.data(proxy.index(0, 1), Qt::DisplayRole).toString(),
              QStringLiteral("a,b"));
}

// data() on a source column routes to the source model (extraCol < 0 branch).
TEST(ListToTableProxyModel, SourceColumnDataRoutesToSource)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}});
    QStandardItem *item = new QStandardItem(QStringLiteral("displayText"));
    item->setData(QStringLiteral("n0"), kNameRole);
    src.appendRow(item);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kNameRole};
    proxy.rolesChanged(proxy.m_roles);

    // Source column 0 -> source DisplayRole data.
    EXPECT_EQ(proxy.data(proxy.index(0, 0), Qt::DisplayRole).toString(),
              QStringLiteral("displayText"));
}

// sourceModelChanged re-titles the extra columns from the new source model's
// role names (the sourceModelChanged lambda / setExtraColumnTitle path).
TEST(ListToTableProxyModel, SourceModelChangedRetitlesColumns)
{
    // Declare the source first so the proxy (declared later) is destroyed
    // first on reverse stack destruction while the source is still alive.
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}});
    src.appendRow(new QStandardItem);

    // Set roles BEFORE plugging a source model: appendColumn() then uses the
    // numeric fallback (no source roleNames available).
    ListToTableProxyModel proxy;
    proxy.m_roles = QList<int>{kNameRole};
    proxy.rolesChanged(proxy.m_roles);

    // Plugging the source fires sourceModelChanged, which re-titles the extra
    // column from the source's role name.
    proxy.setSourceModel(&src);
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("name"));
}

// Changing source data makes the proxy emit dataChanged, which ListToTable
// turns into a model reset (the dataChanged lambda: beginResetModel/endResetModel).
TEST(ListToTableProxyModel, DataChangedTriggersReset)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}});
    QStandardItem *item = new QStandardItem;
    item->setData(QStringLiteral("n0"), kNameRole);
    src.appendRow(item);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kNameRole};
    proxy.rolesChanged(proxy.m_roles);

    QSignalSpy aboutResetSpy(&proxy, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetSpy(&proxy, &QAbstractItemModel::modelReset);
    ASSERT_TRUE(aboutResetSpy.isValid());
    ASSERT_TRUE(resetSpy.isValid());

    // Source data change -> proxy dataChanged (forwarded) -> reset lambda.
    src.setData(src.index(0, 0), QStringLiteral("n0_edited"), kNameRole);

    EXPECT_GE(aboutResetSpy.count(), 1);
    EXPECT_GE(resetSpy.count(), 1);
    // The edited value is still readable through the extra column after reset.
    EXPECT_EQ(proxy.data(proxy.index(0, 1), Qt::DisplayRole).toString(),
              QStringLiteral("n0_edited"));
}

// sourceColumn selects which source column extraColumnData reads from.
TEST(ListToTableProxyModel, SourceColumnSelectsReadColumn)
{
    QStandardItemModel src;
    src.setItemRoleNames({{kNameRole, "name"}});
    // Two explicit items so both source columns exist and are settable.
    src.appendRow({new QStandardItem, new QStandardItem});
    src.setData(src.index(0, 0), QStringLiteral("col0val"), kNameRole);
    src.setData(src.index(0, 1), QStringLiteral("col1val"), kNameRole);

    ListToTableProxyModel proxy;
    proxy.setSourceModel(&src);
    proxy.m_roles = QList<int>{kNameRole};
    proxy.m_sourceColumn = 1; // read from source column 1
    proxy.rolesChanged(proxy.m_roles);

    EXPECT_EQ(proxy.data(proxy.index(0, 2), Qt::DisplayRole).toString(),
              QStringLiteral("col1val"));
}
