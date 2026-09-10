// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <memory>

#include "hoverpreviewproxymodel.h"
#include "sourcemodel.h"

// Helper: create a source model with the given rows.
static std::unique_ptr<TestSourceModel> createSourceModel(std::initializer_list<std::pair<uint32_t, QString>> rows)
{
    auto model = std::make_unique<TestSourceModel>();
    for (auto &r : rows)
        model->addRow(r.first, r.second);
    return model;
}

TEST(HoverPreviewProxyModel, ConstructorSetsDynamicSortFilter)
{
    dock::HoverPreviewProxyModel model;
    // QSortFilterProxyModel::dynamicSortFilter should be true after construction.
    EXPECT_TRUE(model.dynamicSortFilter());
}

TEST(HoverPreviewProxyModel, SetFilterByAppIdMatching)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}, {300, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);

    EXPECT_EQ(proxy.rowCount(), 2);
}

TEST(HoverPreviewProxyModel, SetFilterByAppIdNoMatch)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("nonexistent", dock::HoverPreviewProxyModel::FilterByAppId);

    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, SetFilterByWinIdMatching)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}, {300, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("200", dock::HoverPreviewProxyModel::FilterByWinId);

    EXPECT_EQ(proxy.rowCount(), 1);
    QModelIndex idx = proxy.index(0, 0);
    EXPECT_EQ(idx.data(Qt::UserRole + 1).toUInt(), 200u);
}

TEST(HoverPreviewProxyModel, SetFilterByWinIdNoMatch)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("999", dock::HoverPreviewProxyModel::FilterByWinId);

    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, SetFilterByWinIdZeroFilter)
{
    // targetWinId == 0 should not match anything (the "targetWinId != 0" guard).
    auto source = createSourceModel({{100, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("0", dock::HoverPreviewProxyModel::FilterByWinId);

    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, SetFilterEmptyStringShowsNothing)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("", dock::HoverPreviewProxyModel::FilterByAppId);

    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, ClearFilterShowsNothing)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 1);

    proxy.clearFilter();
    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, SetFilterInvalidatesPreviousFilter)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}, {300, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 2);

    // Switch to filtering by winId
    proxy.setFilter("200", dock::HoverPreviewProxyModel::FilterByWinId);
    EXPECT_EQ(proxy.rowCount(), 1);
}

TEST(HoverPreviewProxyModel, FilterAcceptsRowNoSourceModel)
{
    dock::HoverPreviewProxyModel proxy;
    // No source model set — all rows rejected.
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, WinIdZeroRowRejected)
{
    // A row with winId == 0 should never be accepted regardless of filter.
    auto source = createSourceModel({{0, "app1"}, {100, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);

    EXPECT_EQ(proxy.rowCount(), 1);
    QModelIndex idx = proxy.index(0, 0);
    EXPECT_EQ(idx.data(Qt::UserRole + 1).toUInt(), 100u);
}

TEST(HoverPreviewProxyModel, SwitchFromAppIdToWinIdAndBack)
{
    auto source = createSourceModel({{100, "app1"}, {200, "app2"}, {300, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());

    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 2);

    proxy.setFilter("200", dock::HoverPreviewProxyModel::FilterByWinId);
    EXPECT_EQ(proxy.rowCount(), 1);

    proxy.clearFilter();
    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, SetFilterWithNullSourceModelSafe)
{
    // Should not crash; just shows nothing.
    dock::HoverPreviewProxyModel proxy;
    proxy.setFilter("anything", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 0);
}

TEST(HoverPreviewProxyModel, DynamicSourceModelUpdate)
{
    auto source = createSourceModel({{100, "app1"}});
    dock::HoverPreviewProxyModel proxy;
    proxy.setSourceModel(source.get());
    proxy.setFilter("app1", dock::HoverPreviewProxyModel::FilterByAppId);
    EXPECT_EQ(proxy.rowCount(), 1);

    // Add a new matching row dynamically — proxy should reflect it.
    source->addRow(101, "app1");
    EXPECT_EQ(proxy.rowCount(), 2);

    // Add a non-matching row.
    source->addRow(200, "app2");
    EXPECT_EQ(proxy.rowCount(), 2);
}
