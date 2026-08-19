// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for KExtraColumnsProxyModel (frame/models/kextracolumnsproxymodel),
// a vendored KDE QIdentityProxyModel subclass that appends extra columns.
//
// KExtraColumnsProxyModel is abstract (extraColumnData() is pure virtual), so
// these tests drive it through a minimal concrete subclass defined below. The
// subclass overrides extraColumnData()/setExtraColumnData() with deterministic,
// controlled storage so the base-class branching logic is exercised without
// depending on ListToTableProxyModel's m_roles contract.
//
// Stack declaration order: the source QStandardItemModel is always declared
// BEFORE the proxy so that, on reverse stack destruction, the proxy (declared
// later) is destroyed first while the source is still alive — the same
// convention as tests/panels/dock/taskmanager.

#include <gtest/gtest.h>

#include <QAbstractItemModelTester>
#include <QIdentityProxyModel>
#include <QItemSelection>
#include <QList>
#include <QModelIndex>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "kextracolumnsproxymodel.h"

// Concrete subclass under test: deterministic extra-column storage.
class TestExtraColumnsModel : public KExtraColumnsProxyModel
{
    Q_OBJECT
public:
    explicit TestExtraColumnsModel(QObject *parent = nullptr)
        : KExtraColumnsProxyModel(parent) {}

    QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn,
                             int role = Qt::DisplayRole) const override
    {
        Q_UNUSED(parent)
        Q_UNUSED(row)
        if (role != Qt::DisplayRole)
            return QVariant();
        return m_extraData.value(extraColumn);
    }

    bool setExtraColumnData(const QModelIndex &parent, int row, int extraColumn,
                            const QVariant &data, int role = Qt::EditRole) override
    {
        Q_UNUSED(parent)
        Q_UNUSED(row)
        Q_UNUSED(role)
        m_extraData[extraColumn] = data;
        extraColumnDataChanged(parent, row, extraColumn, {role});
        return true;
    }

    QVariant extraValue(int extraCol) const { return m_extraData.value(extraCol); }

private:
    QHash<int, QVariant> m_extraData;
};

// Custom source model that exposes the (protected) layout-change signals so the
// proxy's _ec_sourceLayout{AboutToBeChanged,Changed} private slots fire. Used
// only by the LayoutChangeHandlers test below.
class TestLayoutSourceModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit TestLayoutSourceModel(QObject *parent = nullptr) : QStandardItemModel(parent) {}
    void emitLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents,
                                    QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint)
    { emit layoutAboutToBeChanged(parents, hint); }
    void emitLayoutChanged(const QList<QPersistentModelIndex> &parents,
                           QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint)
    { emit layoutChanged(parents, hint); }
};

namespace {

// Build a 1-column, N-row source model whose DisplayRole text is "r<row>".
QStandardItemModel *makeSourceModel(int rows, QObject *parent)
{
    auto *src = new QStandardItemModel(parent);
    for (int r = 0; r < rows; ++r)
        src->appendRow(new QStandardItem(QStringLiteral("r%1").arg(r)));
    return src;
}

} // namespace

// appendColumn grows the extra-column count (and therefore columnCount).
TEST(KExtraColumnsProxyModel, AppendColumnGrowsColumnCount)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem); // ensure source has 1 column
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    EXPECT_EQ(proxy.columnCount(), 1); // source has 1 column, no extras yet

    proxy.appendColumn(QStringLiteral("extra1"));
    proxy.appendColumn(QStringLiteral("extra2"));
    EXPECT_EQ(proxy.columnCount(), 1 + 2);
}

// setExtraColumnTitle / removeExtraColumn mutate the extra headers.
TEST(KExtraColumnsProxyModel, SetExtraColumnTitleAndRemove)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem); // ensure source has 1 column
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));
    proxy.appendColumn(QStringLiteral("extra2"));

    proxy.setExtraColumnTitle(0, QStringLiteral("renamed"));
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("renamed"));

    proxy.removeExtraColumn(0);
    EXPECT_EQ(proxy.columnCount(), 1 + 1);
    // The remaining extra column (previously "extra2") is now extra column 0.
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("extra2"));
}

// data() on an extra column routes to extraColumnData; on a source column routes
// to the source model. Covers both branches of KExtraColumnsProxyModel::data.
TEST(KExtraColumnsProxyModel, DataRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("sourceData")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Source column 0 -> source data (extraCol < 0 branch).
    EXPECT_EQ(proxy.data(proxy.index(0, 0), Qt::DisplayRole).toString(),
              QStringLiteral("sourceData"));

    // Extra column 1 -> extraColumnData (extraCol >= 0 && headers non-empty).
    EXPECT_EQ(proxy.data(proxy.index(0, 1), Qt::DisplayRole), QVariant());
    // Non-DisplayRole on extra column -> our override returns invalid.
    EXPECT_FALSE(proxy.data(proxy.index(0, 1), Qt::EditRole).isValid());
}

// setData() on an extra column routes to setExtraColumnData (returns true and
// stores); on a source column routes to the source model. Covers both branches
// of KExtraColumnsProxyModel::setData and exercises setExtraColumnData.
TEST(KExtraColumnsProxyModel, SetDataRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Extra column -> setExtraColumnData (true) + storage + extraColumnDataChanged.
    QSignalSpy changedSpy(&proxy, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(changedSpy.isValid());
    EXPECT_TRUE(proxy.setData(proxy.index(0, 1), QStringLiteral("edited")));
    EXPECT_EQ(proxy.extraValue(0).toString(), QStringLiteral("edited"));
    ASSERT_EQ(changedSpy.count(), 1);

    // Source column -> source model setData.
    EXPECT_TRUE(proxy.setData(proxy.index(0, 0), QStringLiteral("srcEdited")));
    EXPECT_EQ(src.data(src.index(0, 0), Qt::DisplayRole).toString(),
              QStringLiteral("srcEdited"));
}

// flags(): extra columns are read-only Selectable|Enabled; source columns carry
// the source flags; with no source model, flags returns NoItemFlags.
TEST(KExtraColumnsProxyModel, FlagsRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Extra column -> readonly flags (extraCol >= 0 branch).
    EXPECT_EQ(proxy.flags(proxy.index(0, 1)),
              Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // Source column -> source flags (extraCol < 0 branch).
    EXPECT_EQ(proxy.flags(proxy.index(0, 0)), src.flags(src.index(0, 0)));

    // No source model -> NoItemFlags (the `sourceModel() != nullptr` ternary
    // false branch in flags()).
    TestExtraColumnsModel noSourceProxy;
    EXPECT_EQ(noSourceProxy.flags(QModelIndex()), Qt::NoItemFlags);
}

// hasChildren(): column > 0 always false; column 0 defers to the source/base.
TEST(KExtraColumnsProxyModel, HasChildrenRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // column > 0 -> false branch.
    EXPECT_FALSE(proxy.hasChildren(proxy.index(0, 1)));
    // root, column 0 -> base/source hasChildren.
    EXPECT_TRUE(proxy.hasChildren(QModelIndex()));
}

// headerData(): Horizontal+extra+DisplayRole -> extra header text;
// Horizontal+extra+non-DisplayRole -> invalid; Horizontal+source -> source;
// Vertical -> defers to source. Covers all branches of headerData.
TEST(KExtraColumnsProxyModel, HeaderDataRouting)
{
    QStandardItemModel src;
    src.setHorizontalHeaderItem(0, new QStandardItem(QStringLiteral("srcHeader")));
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Horizontal, extra column, DisplayRole -> extra header text.
    EXPECT_EQ(proxy.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("extra1"));
    // Horizontal, extra column, non-DisplayRole -> invalid.
    EXPECT_FALSE(proxy.headerData(1, Qt::Horizontal, Qt::EditRole).isValid());
    // Horizontal, source column -> source headerData.
    EXPECT_EQ(proxy.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("srcHeader"));
    // Vertical -> defers to source headerData (orientation != Horizontal branch).
    // QStandardItemModel's default vertical header for row 0 is "1"; just assert
    // the proxy forwards whatever the source reports.
    EXPECT_EQ(proxy.headerData(0, Qt::Vertical, Qt::DisplayRole),
              src.headerData(0, Qt::Vertical, Qt::DisplayRole));
}

// mapToSource(): invalid proxy index -> invalid; extra column -> invalid;
// source column -> mapped to source. Covers all branches of mapToSource.
TEST(KExtraColumnsProxyModel, MapToSourceRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Invalid proxy index -> invalid (the `!proxyIndex.isValid()` branch).
    EXPECT_FALSE(proxy.mapToSource(QModelIndex()).isValid());

    // Extra column -> invalid (the `column >= columnCount` branch).
    EXPECT_FALSE(proxy.mapToSource(proxy.index(0, 1)).isValid());

    // Source column -> mapped to source.
    QModelIndex mapped = proxy.mapToSource(proxy.index(0, 0));
    ASSERT_TRUE(mapped.isValid());
    EXPECT_EQ(mapped.row(), 0);
    EXPECT_EQ(mapped.column(), 0);
}

// buddy(): extra column returns the proxy index unchanged; source column defers
// to the base implementation. Covers both branches of buddy.
TEST(KExtraColumnsProxyModel, BuddyRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    QModelIndex extraIdx = proxy.index(0, 1);
    EXPECT_EQ(proxy.buddy(extraIdx), extraIdx); // extra column branch

    QModelIndex srcIdx = proxy.index(0, 0);
    EXPECT_EQ(proxy.buddy(srcIdx), srcIdx); // source column: buddy is self
}

// sibling(): identical row+column returns the index itself; different returns a
// freshly created index. Covers both branches of sibling.
TEST(KExtraColumnsProxyModel, SiblingRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    QModelIndex idx = proxy.index(0, 1);
    // Same row/column -> returns idx (the `row==idx.row() && column==idx.column()` branch).
    EXPECT_EQ(proxy.sibling(0, 1, idx), idx);
    // Different column -> newly created index (the else branch).
    QModelIndex sib = proxy.sibling(0, 0, idx);
    ASSERT_TRUE(sib.isValid());
    EXPECT_EQ(sib.row(), 0);
    EXPECT_EQ(sib.column(), 0);
}

// mapSelectionToSource(): with a source model, a selection spanning the extra
// columns is truncated to the source column range; without a source model an
// empty selection is returned. Covers both branches.
TEST(KExtraColumnsProxyModel, MapSelectionToSourceRouting)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));
    proxy.appendColumn(QStringLiteral("extra2"));

    // Selection spanning source col 0 through extra col 2 (proxy cols 0..2).
    QItemSelection sel(proxy.index(0, 0), proxy.index(0, 2));
    QItemSelection mapped = proxy.mapSelectionToSource(sel);
    ASSERT_EQ(mapped.size(), 1);
    // bottomRight column (2) >= sourceColumnCount (1) -> truncated to col 0.
    EXPECT_EQ(mapped.at(0).bottomRight().column(), 0);
    EXPECT_EQ(mapped.at(0).topLeft().column(), 0);

    // No source model -> empty selection (the `!sourceModel()` branch).
    TestExtraColumnsModel noSourceProxy;
    QItemSelection emptySel;
    EXPECT_TRUE(noSourceProxy.mapSelectionToSource(emptySel).isEmpty());
}

// extraColumnForProxyColumn / proxyColumnForExtraColumn: the mapping helpers
// behave with and without a source model.
TEST(KExtraColumnsProxyModel, ColumnMappingHelpers)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // With a source model: proxy column below source count -> -1.
    EXPECT_EQ(proxy.extraColumnForProxyColumn(0), -1);
    // proxy column at/above source count -> extra column index.
    EXPECT_EQ(proxy.extraColumnForProxyColumn(1), 0);
    EXPECT_EQ(proxy.proxyColumnForExtraColumn(0), 1);

    // Without a source model: always -1.
    TestExtraColumnsModel noSourceProxy;
    EXPECT_EQ(noSourceProxy.extraColumnForProxyColumn(0), -1);
    EXPECT_EQ(noSourceProxy.extraColumnForProxyColumn(5), -1);
}

// index()/parent(): extra-column indexes are created with the source column-0
// internal pointer; parent() of an extra-column child resolves via a column-0
// sibling. Covers the extra-column branches of index() and parent().
TEST(KExtraColumnsProxyModel, IndexAndParentForExtraColumns)
{
    QStandardItemModel src;
    QStandardItem *parent = new QStandardItem(QStringLiteral("parent"));
    src.appendRow(parent);
    // Give the parent a child row so a hierarchical index exists.
    QStandardItem *child = new QStandardItem(QStringLiteral("child"));
    parent->setChild(0, child);
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Top-level extra-column index is valid (extra-col >= 0 branch of index()).
    QModelIndex extraTop = proxy.index(0, 1);
    EXPECT_TRUE(extraTop.isValid());
    EXPECT_EQ(extraTop.column(), 1);

    // Top-level extra-column index parent is invalid (extra-col branch of parent()).
    EXPECT_FALSE(proxy.parent(extraTop).isValid());

    // Child extra-column index parent is the parent row (column 0).
    QModelIndex childExtra = proxy.index(0, 1, proxy.index(0, 0));
    ASSERT_TRUE(childExtra.isValid());
    QModelIndex childParent = proxy.parent(childExtra);
    ASSERT_TRUE(childParent.isValid());
    EXPECT_EQ(childParent.row(), 0);
    EXPECT_EQ(childParent.column(), 0);
}

// extraColumnDataChanged() emits dataChanged for the given extra column.
TEST(KExtraColumnsProxyModel, ExtraColumnDataChangedEmits)
{
    QStandardItemModel src;
    src.appendRow(new QStandardItem(QStringLiteral("src")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    QSignalSpy spy(&proxy, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(spy.isValid());

    proxy.extraColumnDataChanged(QModelIndex(), 0, 0, {Qt::DisplayRole});
    ASSERT_EQ(spy.count(), 1);
    const auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toModelIndex().column(), 1); // proxyColumnForExtraColumn(0)
    EXPECT_EQ(args.at(1).toModelIndex().column(), 1);
}

// setSourceModel twice: replacing the source exercises the disconnect-old /
// connect-new branches of setSourceModel.
TEST(KExtraColumnsProxyModel, ReplaceSourceModel)
{
    QStandardItemModel src1;
    QStandardItemModel src2;
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src1);
    EXPECT_EQ(proxy.sourceModel(), &src1);
    // Replacing triggers the `if (sourceModel())` disconnect branch and the
    // `if (model)` connect branch.
    proxy.setSourceModel(&src2);
    EXPECT_EQ(proxy.sourceModel(), &src2);
    // Clearing triggers the disconnect branch again, no connect (model == null).
    proxy.setSourceModel(nullptr);
    EXPECT_EQ(proxy.sourceModel(), nullptr);
}

// QAbstractItemModelTester validates model invariants across extra-column
// operations on a flat source model. (Source is heap-allocated and parented to
// the proxy so it is destroyed with the proxy.)
TEST(KExtraColumnsProxyModel, ModelTesterFlatValidation)
{
    TestExtraColumnsModel proxy;
    auto *src = makeSourceModel(3, &proxy);
    proxy.appendColumn(QStringLiteral("extra1"));
    proxy.appendColumn(QStringLiteral("extra2"));
    proxy.setSourceModel(src);

    auto tester = std::make_unique<QAbstractItemModelTester>(
        &proxy, QAbstractItemModelTester::FailureReportingMode::Fatal);

    // Mutations that touch both source and extra columns.
    proxy.setData(proxy.index(1, 0), QStringLiteral("edited"));
    proxy.setData(proxy.index(1, 1), QStringLiteral("extraEdited"));
    src->removeRow(0);
    src->appendRow(new QStandardItem(QStringLiteral("r_new")));

    EXPECT_GT(proxy.rowCount(), 0);
}

// The _ec_sourceLayoutAboutToBeChanged / _ec_sourceLayoutChanged handlers fire
// when the source emits layoutAboutToBeChanged/layoutChanged. With persistent
// proxy indexes on both a source column and an extra column, the handlers'
// persistent-index loop runs and exercises both the `column < sourceColumnCount`
// and `column >= sourceColumnCount` branches. An invalid source parent hits the
// `!parent.isValid()` branch; a valid source parent hits the mapFromSource branch.
TEST(KExtraColumnsProxyModel, LayoutChangeHandlers)
{
    TestLayoutSourceModel src;
    src.appendRow(new QStandardItem(QStringLiteral("r0")));
    src.appendRow(new QStandardItem(QStringLiteral("r1")));
    TestExtraColumnsModel proxy;
    proxy.setSourceModel(&src);
    proxy.appendColumn(QStringLiteral("extra1"));

    // Persistent proxy indexes: one on a source column, one on an extra column.
    QPersistentModelIndex persSource(proxy.index(0, 0));
    QPersistentModelIndex persExtra(proxy.index(0, 1));
    ASSERT_TRUE(persSource.isValid());
    ASSERT_TRUE(persExtra.isValid());

    // Cycle 1: an invalid source parent -> the `!parent.isValid()` branch.
    QList<QPersistentModelIndex> invalidParents{QPersistentModelIndex()};
    src.emitLayoutAboutToBeChanged(invalidParents);
    src.emitLayoutChanged(invalidParents);

    // Persistent indexes survive a well-formed (no-op) layout change.
    EXPECT_TRUE(persSource.isValid());
    EXPECT_TRUE(persExtra.isValid());

    // Cycle 2: a valid source parent -> the mapFromSource branch.
    QList<QPersistentModelIndex> validParents{QPersistentModelIndex(src.index(0, 0))};
    src.emitLayoutAboutToBeChanged(validParents);
    src.emitLayoutChanged(validParents);

    EXPECT_TRUE(persSource.isValid());
    EXPECT_TRUE(persExtra.isValid());
}

#include "kextracolumnsproxymodeltests.moc"
