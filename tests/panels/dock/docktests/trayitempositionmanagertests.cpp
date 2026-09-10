// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QPoint>
#include <QSize>
#include <QTimer>
#include <QTest>

#include "trayitempositionmanager.h"

using namespace docktray;

// Helper: reset singleton to a known state before each test.
// The singleton persists across tests, so we must clear its mutable state.
static void resetSingleton()
{
    auto &mgr = TrayItemPositionManager::instance();
    mgr.clearRegisteredSizes();      // clear registered items (no-op if empty)
    mgr.m_dockHeight = 0;            // prevent updateVisualSize side effects
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 0;
    mgr.m_visualSize = QSize();
}

// =================== create() / instance() ===================

TEST(TrayItemPositionManager, InstanceReturnsSameReference)
{
    auto &a = TrayItemPositionManager::instance();
    auto &b = TrayItemPositionManager::instance();
    EXPECT_EQ(&a, &b);
}

TEST(TrayItemPositionManager, CreateReturnsInstancePointer)
{
    auto *p = TrayItemPositionManager::create(nullptr, nullptr);
    EXPECT_EQ(p, &TrayItemPositionManager::instance());
}

// =================== Constructor defaults ===================

TEST(TrayItemPositionManager, ConstructorSetsItemSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    EXPECT_EQ(mgr.m_itemSpacing, 2);
}

TEST(TrayItemPositionManager, ConstructorSetsItemPadding)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    EXPECT_EQ(mgr.m_itemPadding, 4);
}

TEST(TrayItemPositionManager, ConstructorSetsItemVisualSize)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    EXPECT_EQ(mgr.m_itemVisualSize, QSize(24, 24));
}

// =================== orientation() / dockHeight() ===================

TEST(TrayItemPositionManager, OrientationReturnsValue)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    EXPECT_EQ(mgr.orientation(), Qt::Vertical);
}

TEST(TrayItemPositionManager, DockHeightReturnsValue)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 50;
    EXPECT_EQ(mgr.dockHeight(), 50);
}

// =================== registerVisualItemSize ===================

TEST(TrayItemPositionManager, RegisterVisualItemSizeExtendsWithDefaults)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(2, QSize(10, 10));
    // Items 0 and 1 should have default size (24,24), item 2 has (10,10).
    EXPECT_EQ(mgr.visualItemSize(0), QSize(24, 24));
    EXPECT_EQ(mgr.visualItemSize(1), QSize(24, 24));
    EXPECT_EQ(mgr.visualItemSize(2), QSize(10, 10));
}

TEST(TrayItemPositionManager, RegisterVisualItemSizeEmitsWhenChanged)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.registerVisualItemSize(0, QSize(10, 10));
    EXPECT_EQ(spy.count(), 1);
}

TEST(TrayItemPositionManager, RegisterVisualItemSizeNoEmitWhenSame)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    // First register with default size — item is extended with default,
    // then replaced with same default → no change → no emit.
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.registerVisualItemSize(0, QSize(24, 24));
    EXPECT_EQ(spy.count(), 0);
}

TEST(TrayItemPositionManager, RegisterVisualItemSizeUpdateExisting)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(10, 10));
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.registerVisualItemSize(0, QSize(20, 20));
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(mgr.visualItemSize(0), QSize(20, 20));
}

TEST(TrayItemPositionManager, RegisterVisualItemSizeNoEmitWhenSameUpdate)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(10, 10));
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.registerVisualItemSize(0, QSize(10, 10));
    EXPECT_EQ(spy.count(), 0);
}

TEST(TrayItemPositionManager, RegisterVisualItemSizeIndexZero)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(16, 16));
    EXPECT_EQ(mgr.visualItemSize(0), QSize(16, 16));
    EXPECT_EQ(mgr.m_registeredItemsSize.count(), 1);
}

// =================== visualItemSize ===================

TEST(TrayItemPositionManager, VisualItemSizeUnregisteredReturnsDefault)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    EXPECT_EQ(mgr.visualItemSize(0), QSize(24, 24));
    EXPECT_EQ(mgr.visualItemSize(5), QSize(24, 24));
    EXPECT_EQ(mgr.visualItemSize(100), QSize(24, 24));
}

TEST(TrayItemPositionManager, VisualItemSizeRegisteredReturnsValue)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(16, 16));
    mgr.registerVisualItemSize(1, QSize(32, 32));
    EXPECT_EQ(mgr.visualItemSize(0), QSize(16, 16));
    EXPECT_EQ(mgr.visualItemSize(1), QSize(32, 32));
    // Unregistered index still returns default.
    EXPECT_EQ(mgr.visualItemSize(2), QSize(24, 24));
}

// =================== visualSize (horizontal) ===================

TEST(TrayItemPositionManager, VisualSizeHorizontalIndexZeroIncludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_dockHeight = 50;
    // width = 24 + 2 = 26, includeLastSpacing=true, index=0
    EXPECT_EQ(mgr.visualSize(0, true), QSize(26, 50));
}

TEST(TrayItemPositionManager, VisualSizeHorizontalIndexZeroExcludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_dockHeight = 50;
    // index=0 → !includeLastSpacing && index>0 is false → width stays 26
    EXPECT_EQ(mgr.visualSize(0, false), QSize(26, 50));
}

TEST(TrayItemPositionManager, VisualSizeHorizontalMultipleItemsIncludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_dockHeight = 50;
    mgr.registerVisualItemSize(0, QSize(10, 10));
    mgr.registerVisualItemSize(1, QSize(20, 20));
    // width = (10+2) + (20+2) = 34, includeLastSpacing=true
    EXPECT_EQ(mgr.visualSize(1, true), QSize(34, 50));
}

TEST(TrayItemPositionManager, VisualSizeHorizontalMultipleItemsExcludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_dockHeight = 50;
    mgr.registerVisualItemSize(0, QSize(10, 10));
    mgr.registerVisualItemSize(1, QSize(20, 20));
    // width = (10+2) + (20+2) = 34, exclude last spacing → 34-2 = 32
    EXPECT_EQ(mgr.visualSize(1, false), QSize(32, 50));
}

// =================== visualSize (vertical) ===================

TEST(TrayItemPositionManager, VisualSizeVerticalIndexZeroIncludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_dockHeight = 50;
    // height = 24 + 2 = 26, includeLastSpacing=true, index=0
    EXPECT_EQ(mgr.visualSize(0, true), QSize(50, 26));
}

TEST(TrayItemPositionManager, VisualSizeVerticalIndexZeroExcludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_dockHeight = 50;
    // index=0 → !includeLastSpacing && index>0 is false → height stays 26
    EXPECT_EQ(mgr.visualSize(0, false), QSize(50, 26));
}

TEST(TrayItemPositionManager, VisualSizeVerticalMultipleItemsExcludeSpacing)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_dockHeight = 50;
    mgr.registerVisualItemSize(0, QSize(10, 10));
    mgr.registerVisualItemSize(1, QSize(20, 20));
    // height = (10+2) + (20+2) = 34, exclude last → 34-2 = 32
    EXPECT_EQ(mgr.visualSize(1, false), QSize(50, 32));
}

// =================== itemIndexByPoint (horizontal) ===================

TEST(TrayItemPositionManager, ItemIndexByPointHorizontalOnFirstItem)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 3;
    // Item 0: width 0..25 (24+2=26 boundary). Point at x=5 is on item 0.
    DropIndex result = mgr.itemIndexByPoint(QPoint(5, 0));
    EXPECT_EQ(result.index, 0);
    EXPECT_TRUE(result.isOnItem);   // pos=5 <= 24
    EXPECT_TRUE(result.isBefore);   // pos=5 < 12
}

TEST(TrayItemPositionManager, ItemIndexByPointHorizontalAfterHalf)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 3;
    // pos=15 is > 12 (half of 24) → isBefore=false, still on item (15 <= 24)
    DropIndex result = mgr.itemIndexByPoint(QPoint(15, 0));
    EXPECT_EQ(result.index, 0);
    EXPECT_TRUE(result.isOnItem);
    EXPECT_FALSE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointHorizontalSecondItem)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 3;
    // Item 1 starts at width=26. Point at x=30: pos < 26+24+2=52 → yes.
    // pos -= 26 → 4. isOnItem: 4<=24 true. isBefore: 4<12 true.
    DropIndex result = mgr.itemIndexByPoint(QPoint(30, 0));
    EXPECT_EQ(result.index, 1);
    EXPECT_TRUE(result.isOnItem);
    EXPECT_TRUE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointHorizontalBeyondAllItems)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 3;
    // pos=200 is beyond all items → fallback {index = m_visualItemCount - 1 = 2}
    DropIndex result = mgr.itemIndexByPoint(QPoint(200, 0));
    EXPECT_EQ(result.index, 2);
    // Default values: isOnItem=true, isBefore=false
    EXPECT_TRUE(result.isOnItem);
    EXPECT_FALSE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointHorizontalZeroItems)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 0;
    // Loop never executes, fallback {index = -1}
    DropIndex result = mgr.itemIndexByPoint(QPoint(5, 0));
    EXPECT_EQ(result.index, -1);
}

// =================== itemIndexByPoint (vertical) ===================

TEST(TrayItemPositionManager, ItemIndexByPointVerticalOnFirstItem)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_visualItemCount = 3;
    DropIndex result = mgr.itemIndexByPoint(QPoint(0, 5));
    EXPECT_EQ(result.index, 0);
    EXPECT_TRUE(result.isOnItem);
    EXPECT_TRUE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointVerticalAfterHalf)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_visualItemCount = 3;
    DropIndex result = mgr.itemIndexByPoint(QPoint(0, 15));
    EXPECT_EQ(result.index, 0);
    EXPECT_TRUE(result.isOnItem);
    EXPECT_FALSE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointVerticalSecondItem)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_visualItemCount = 3;
    DropIndex result = mgr.itemIndexByPoint(QPoint(0, 30));
    EXPECT_EQ(result.index, 1);
    EXPECT_TRUE(result.isOnItem);
    EXPECT_TRUE(result.isBefore);
}

TEST(TrayItemPositionManager, ItemIndexByPointVerticalBeyondAllItems)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_visualItemCount = 3;
    // Vertical loop uses i <= m_visualItemCount (off-by-one vs horizontal).
    // With 3 items: i goes 0,1,2,3. At i=3, visualItemSize(3)=default(24,24).
    // width accumulated: (24+2)*3 = 78. Boundary: 78+24+2=104.
    // pos=200 > 104 → fallback {index = 2}
    DropIndex result = mgr.itemIndexByPoint(QPoint(0, 200));
    EXPECT_EQ(result.index, 2);
}

TEST(TrayItemPositionManager, ItemIndexByPointVerticalExtraIteration)
{
    // DEFECT: vertical loop uses i <= m_visualItemCount instead of i < m_visualItemCount.
    // This means with 3 items, a point at the 4th phantom position (y=80..103)
    // returns index=3 instead of falling back to index=2.
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_orientation = Qt::Vertical;
    mgr.m_visualItemCount = 3;
    // After 3 items, accumulated height = (24+2)*3 = 78.
    // i=3: pos < 78+24+2=104. pos=80 < 104 → returns index=3.
    DropIndex result = mgr.itemIndexByPoint(QPoint(0, 80));
    EXPECT_EQ(result.index, 3);
}

// =================== clearRegisteredSizes ===================

TEST(TrayItemPositionManager, ClearRegisteredSizesEmpty)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    // Nothing to clear — should not emit.
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.clearRegisteredSizes();
    EXPECT_EQ(spy.count(), 0);
}

TEST(TrayItemPositionManager, ClearRegisteredSizesNonEmptyEmits)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(10, 10));
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.clearRegisteredSizes();
    EXPECT_EQ(spy.count(), 1);
    // After clearing, visualItemSize returns default.
    EXPECT_EQ(mgr.visualItemSize(0), QSize(24, 24));
    EXPECT_TRUE(mgr.m_registeredItemsSize.isEmpty());
}

TEST(TrayItemPositionManager, ClearRegisteredSizesIdempotent)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.registerVisualItemSize(0, QSize(10, 10));
    mgr.clearRegisteredSizes();
    // Second call on empty list is a no-op.
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualItemSizeChanged);
    mgr.clearRegisteredSizes();
    EXPECT_EQ(spy.count(), 0);
}

// =================== layoutHealthCheck ===================

TEST(TrayItemPositionManager, LayoutHealthCheckDockHeightZero)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 0;
    QSignalSpy spy(&mgr, &TrayItemPositionManager::orientationChanged);
    mgr.layoutHealthCheck(0);
    // Timer fires, but dockHeight==0 → early return, no signal.
    EXPECT_FALSE(spy.wait(1000));
    EXPECT_EQ(spy.count(), 0);
}

TEST(TrayItemPositionManager, LayoutHealthCheckSizeMismatch)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 50;
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 1;
    // visualSize(0, false) = QSize(26, 50). Set m_visualSize to something else.
    mgr.m_visualSize = QSize(0, 0);
    QSignalSpy spy(&mgr, &TrayItemPositionManager::orientationChanged);
    mgr.layoutHealthCheck(0);
    EXPECT_TRUE(spy.wait(1000));
    EXPECT_EQ(spy.count(), 1);
}

TEST(TrayItemPositionManager, LayoutHealthCheckSizeMatch)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 50;
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 1;
    // Set m_visualSize to match visualSize(0, false) = QSize(26, 50).
    mgr.m_visualSize = QSize(26, 50);
    QSignalSpy spy(&mgr, &TrayItemPositionManager::orientationChanged);
    mgr.layoutHealthCheck(0);
    // Sizes match → no orientationChanged signal.
    EXPECT_FALSE(spy.wait(1000));
    EXPECT_EQ(spy.count(), 0);
}

TEST(TrayItemPositionManager, LayoutHealthCheckCustomDelay)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 50;
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 1;
    mgr.m_visualSize = QSize(0, 0);
    QSignalSpy spy(&mgr, &TrayItemPositionManager::orientationChanged);
    mgr.layoutHealthCheck(50);  // 50ms delay
    EXPECT_TRUE(spy.wait(2000));
    EXPECT_EQ(spy.count(), 1);
}

// =================== updateVisualSize (via signal connections) ===================

TEST(TrayItemPositionManager, UpdateVisualSizeDockHeightZeroNoOp)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 0;
    mgr.m_visualSize = QSize(99, 99);
    // Emitting visualItemSizeChanged triggers updateVisualSize,
    // but dockHeight==0 → early return → m_visualSize unchanged.
    emit mgr.visualItemSizeChanged();
    EXPECT_EQ(mgr.m_visualSize, QSize(99, 99));
}

TEST(TrayItemPositionManager, UpdateVisualSizeSetsVisualSize)
{
    resetSingleton();
    auto &mgr = TrayItemPositionManager::instance();
    mgr.m_dockHeight = 50;
    mgr.m_orientation = Qt::Horizontal;
    mgr.m_visualItemCount = 1;
    mgr.m_visualSize = QSize(0, 0);
    // Emitting visualItemSizeChanged triggers updateVisualSize.
    // visualSize(0, false) = QSize(26, 50) → setProperty sets m_visualSize.
    QSignalSpy spy(&mgr, &TrayItemPositionManager::visualSizeChanged);
    emit mgr.visualItemSizeChanged();
    EXPECT_EQ(mgr.m_visualSize, QSize(26, 50));
    EXPECT_GE(spy.count(), 1);
}

// =================== DropIndex struct defaults ===================

TEST(TrayItemPositionManager, DropIndexDefaults)
{
    DropIndex di;
    EXPECT_TRUE(di.isOnItem);
    EXPECT_FALSE(di.isBefore);
}

TEST(TrayItemPositionManager, DropIndexFieldAssignment)
{
    DropIndex di;
    di.index = 5;
    di.isOnItem = false;
    di.isBefore = true;
    EXPECT_EQ(di.index, 5);
    EXPECT_FALSE(di.isOnItem);
    EXPECT_TRUE(di.isBefore);
}

// Custom main: QCoreApplication required so QSignalSpy::wait() / QTimer::singleShot
// have an event loop to process. GTest::Main does not create a QCoreApplication.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
