// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for ItemsPage (applets/dde-apps/itemspage), a pure-logic
// pagination manager. Exercises all public methods and their branches:
// construction, name, pageCount, append/insert/move/remove items, findItem,
// contains, allArrangedItems, allPagedItems, itemCount, removeEmptyPages,
// removeItemsNotIn, firstNItems. No external deps (pure QObject).

#include <gtest/gtest.h>

#include <QSet>
#include <QString>
#include <QStringList>
#include <QSignalSpy>

#include "itemspage.h"

// --- Construction & basic properties ---

TEST(ItemsPage, ConstructWithMaxCount)
{
    ItemsPage page(4);
    EXPECT_EQ(page.maxItemCountPerPage(), 4);
    EXPECT_EQ(page.pageCount(), 0);
    EXPECT_TRUE(page.name().isEmpty());
}

TEST(ItemsPage, ConstructWithNameAndMaxCount)
{
    ItemsPage page(QStringLiteral("mygroup"), 3);
    EXPECT_EQ(page.maxItemCountPerPage(), 3);
    EXPECT_EQ(page.name(), QStringLiteral("mygroup"));
    EXPECT_EQ(page.pageCount(), 0);
}

// --- name / setName ---

TEST(ItemsPage, SetNameEmitsSignal)
{
    ItemsPage page(4);
    QSignalSpy spy(&page, &ItemsPage::nameChanged);
    ASSERT_TRUE(spy.isValid());
    page.setName(QStringLiteral("newname"));
    EXPECT_EQ(page.name(), QStringLiteral("newname"));
    EXPECT_EQ(spy.count(), 1);
}

// --- appendEmptyPage ---

TEST(ItemsPage, AppendEmptyPage)
{
    ItemsPage page(3);
    QSignalSpy pageSpy(&page, &ItemsPage::pageCountChanged);
    QSignalSpy addedSpy(&page, &ItemsPage::sigPageAdded);
    page.appendEmptyPage();
    EXPECT_EQ(page.pageCount(), 1);
    EXPECT_EQ(pageSpy.count(), 1);
    EXPECT_EQ(addedSpy.count(), 1);
    EXPECT_TRUE(page.items(0).isEmpty());
}

// --- appendPage ---

TEST(ItemsPage, AppendPageExactMultiple)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    EXPECT_EQ(page.pageCount(), 1);
    EXPECT_EQ(page.items(0).size(), 3);
}

TEST(ItemsPage, AppendPageWithRemainder)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"),
                     QStringLiteral("d")});
    EXPECT_EQ(page.pageCount(), 2);
    EXPECT_EQ(page.items(0).size(), 3);
    EXPECT_EQ(page.items(1).size(), 1);
    EXPECT_EQ(page.items(1).first(), QStringLiteral("d"));
}

TEST(ItemsPage, AppendPageEmptyIsNoop)
{
    ItemsPage page(3);
    page.appendPage({});
    EXPECT_EQ(page.pageCount(), 0);
}

TEST(ItemsPage, AppendPageSignals)
{
    ItemsPage page(2);
    QSignalSpy countSpy(&page, &ItemsPage::pageCountChanged);
    QSignalSpy addedSpy(&page, &ItemsPage::sigPageAdded);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d")});
    // 4 items / 2 per page = 2 pages, exact multiple (no remainder branch)
    EXPECT_EQ(page.pageCount(), 2);
    EXPECT_EQ(countSpy.count(), 1);
    EXPECT_EQ(addedSpy.count(), 1);
}

// --- appendItem (find a page with empty place) ---

TEST(ItemsPage, AppendItemToEmptyPage)
{
    ItemsPage page(3);
    page.appendEmptyPage();
    page.appendItem(QStringLiteral("a"));
    EXPECT_EQ(page.items(0).size(), 1);
    EXPECT_EQ(page.items(0).first(), QStringLiteral("a"));
}

TEST(ItemsPage, AppendItemFillsExistingPage)
{
    ItemsPage page(2);
    page.appendEmptyPage();
    page.appendItem(QStringLiteral("a"));
    page.appendItem(QStringLiteral("b"));
    EXPECT_EQ(page.items(0).size(), 2);
    // Next append should create a new page
    page.appendItem(QStringLiteral("c"));
    EXPECT_EQ(page.pageCount(), 2);
    EXPECT_EQ(page.items(1).first(), QStringLiteral("c"));
}

TEST(ItemsPage, AppendItemNoExistingPages)
{
    ItemsPage page(2);
    // No pages exist → appendPage with the item
    page.appendItem(QStringLiteral("a"));
    EXPECT_EQ(page.pageCount(), 1);
    EXPECT_EQ(page.items(0).first(), QStringLiteral("a"));
}

// --- insertItem ---

TEST(ItemsPage, InsertItemInMiddle)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    page.insertItem(QStringLiteral("x"), 0, 1); // insert at page 0, pos 1
    EXPECT_EQ(page.items(0).at(1), QStringLiteral("x"));
    // Page now has 4 items > max 3 → last spills to new page
    EXPECT_EQ(page.pageCount(), 2);
    EXPECT_EQ(page.items(1).first(), QStringLiteral("c"));
}

TEST(ItemsPage, InsertItemAtEnd)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.insertItem(QStringLiteral("c"), 0, 5); // pos > count → clamped to count
    EXPECT_EQ(page.items(0).size(), 3);
    EXPECT_EQ(page.items(0).last(), QStringLiteral("c"));
}

TEST(ItemsPage, InsertItemSpillsToNextPage)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.insertItem(QStringLiteral("x"), 0, 0); // insert at front, full page
    // "b" spills to next page (which doesn't exist → appendPage)
    EXPECT_EQ(page.pageCount(), 2);
    EXPECT_EQ(page.items(0).at(0), QStringLiteral("x"));
    EXPECT_EQ(page.items(0).at(1), QStringLiteral("a"));
    EXPECT_EQ(page.items(1).first(), QStringLiteral("b"));
}

// --- insertItemToPage ---

TEST(ItemsPage, InsertItemToPage)
{
    ItemsPage page(3);
    page.appendEmptyPage();
    page.appendEmptyPage();
    page.insertItemToPage(QStringLiteral("x"), 1);
    EXPECT_EQ(page.items(1).size(), 1);
    EXPECT_EQ(page.items(1).first(), QStringLiteral("x"));
}

// --- findItem / contains ---

TEST(ItemsPage, FindItemExists)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    page.appendPage({QStringLiteral("d"), QStringLiteral("e")});
    auto [pageIdx, itemIdx] = page.findItem(QStringLiteral("d"));
    EXPECT_EQ(pageIdx, 1);
    EXPECT_EQ(itemIdx, 0);
}

TEST(ItemsPage, FindItemNotFound)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a")});
    auto [pageIdx, itemIdx] = page.findItem(QStringLiteral("z"));
    EXPECT_EQ(pageIdx, -1);
    EXPECT_EQ(itemIdx, -1);
}

TEST(ItemsPage, Contains)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    EXPECT_TRUE(page.contains(QStringLiteral("a")));
    EXPECT_TRUE(page.contains(QStringLiteral("b")));
    EXPECT_FALSE(page.contains(QStringLiteral("z")));
}

// --- removeItem ---

TEST(ItemsPage, RemoveItemKeepsPage)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    EXPECT_TRUE(page.removeItem(QStringLiteral("b")));
    EXPECT_EQ(page.items(0).size(), 2);
    EXPECT_FALSE(page.contains(QStringLiteral("b")));
}

TEST(ItemsPage, RemoveItemRemovesEmptyPage)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a")});
    EXPECT_TRUE(page.removeItem(QStringLiteral("a")));
    EXPECT_EQ(page.pageCount(), 0); // page was emptied → removed
}

TEST(ItemsPage, RemoveItemKeepEmptyPage)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a")});
    EXPECT_TRUE(page.removeItem(QStringLiteral("a"), false)); // don't remove empty page
    EXPECT_EQ(page.pageCount(), 1);
    EXPECT_TRUE(page.items(0).isEmpty());
}

TEST(ItemsPage, RemoveItemNotFoundReturnsFalse)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a")});
    EXPECT_FALSE(page.removeItem(QStringLiteral("z")));
}

// --- removeEmptyPages ---

TEST(ItemsPage, RemoveEmptyPages)
{
    ItemsPage page(3);
    page.appendEmptyPage();
    page.appendEmptyPage();
    page.appendPage({QStringLiteral("a")});
    EXPECT_EQ(page.pageCount(), 3);
    page.removeEmptyPages();
    EXPECT_EQ(page.pageCount(), 1);
}

TEST(ItemsPage, RemoveEmptyPagesNoChange)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a")});
    QSignalSpy spy(&page, &ItemsPage::pageCountChanged);
    page.removeEmptyPages();
    EXPECT_EQ(page.pageCount(), 1);
    EXPECT_EQ(spy.count(), 0); // no change → no signal
}

// --- removeItemsNotIn ---

TEST(ItemsPage, RemoveItemsNotIn)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("internal/x")});
    page.removeItemsNotIn(QSet<QString>{QStringLiteral("a"), QStringLiteral("internal/x")});
    EXPECT_EQ(page.items(0).size(), 2);
    EXPECT_TRUE(page.contains(QStringLiteral("a")));
    EXPECT_FALSE(page.contains(QStringLiteral("b")));
    // internal/ items are always kept
    EXPECT_TRUE(page.contains(QStringLiteral("internal/x")));
}

TEST(ItemsPage, RemoveItemsNotInAllRemovedClearsPages)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.removeItemsNotIn(QSet<QString>{QStringLiteral("z")});
    EXPECT_EQ(page.pageCount(), 0); // all removed, empty pages cleaned
}

// --- moveItemPosition ---

TEST(ItemsPage, MoveItemSamePageForward)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    // Move "a" (page 0, index 0) to page 0, index 2, appendToIndexItem=true
    page.moveItemPosition(0, 0, 0, 2, true);
    // fromIndex(0) > toIndex(2) is false (0 < 2), so toIndex stays 2
    EXPECT_EQ(page.items(0).at(2), QStringLiteral("a"));
}

TEST(ItemsPage, MoveItemSamePageAdjacentAppendIsNoop)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    // fromIndex=1, toIndex=0, adjacent (fromIndex == toIndex+1) and append → do nothing
    page.moveItemPosition(0, 1, 0, 0, true);
    // No change: a stays at index 0
    EXPECT_EQ(page.items(0).at(0), QStringLiteral("a"));
    EXPECT_EQ(page.items(0).at(1), QStringLiteral("b"));
}

TEST(ItemsPage, MoveItemCrossPage)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c"), QStringLiteral("d")});
    // Move "a" from page 0 index 0 to page 1 index 0
    page.moveItemPosition(0, 0, 1, 0, false);
    EXPECT_FALSE(page.items(0).contains(QStringLiteral("a")));
    EXPECT_TRUE(page.items(1).contains(QStringLiteral("a")));
}

TEST(ItemsPage, MoveItemRemovesEmptySourcePage)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a")});
    page.appendPage({QStringLiteral("b"), QStringLiteral("c")});
    // Source page has only 1 item → after move, it becomes empty → removed
    page.moveItemPosition(0, 0, 1, 0, false);
    EXPECT_EQ(page.pageCount(), 2); // source page removed, dest spilled to new page
    EXPECT_EQ(page.items(0), (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
    EXPECT_EQ(page.items(1), (QStringList{QStringLiteral("c")}));
}

// --- allArrangedItems / allPagedItems / itemCount ---

TEST(ItemsPage, AllArrangedItems)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c")});
    EXPECT_EQ(page.allArrangedItems(), (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
}

TEST(ItemsPage, AllPagedItems)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c")});
    auto paged = page.allPagedItems();
    EXPECT_EQ(paged.size(), 2);
    EXPECT_EQ(paged[0], QStringList({QStringLiteral("a"), QStringLiteral("b")}));
    EXPECT_EQ(paged[1], QStringList({QStringLiteral("c")}));
}

TEST(ItemsPage, ItemCountTotal)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c")});
    EXPECT_EQ(page.itemCount(), 3);
}

TEST(ItemsPage, ItemCountByPage)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c")});
    EXPECT_EQ(page.itemCount(0), 2);
    EXPECT_EQ(page.itemCount(1), 1);
    EXPECT_EQ(page.itemCount(99), 0); // out of range → 0
}

// --- firstNItems ---

TEST(ItemsPage, FirstNItems)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c"), QStringLiteral("d")});
    EXPECT_EQ(page.firstNItems(2), (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
    EXPECT_EQ(page.firstNItems(3), (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
    EXPECT_EQ(page.firstNItems(10), (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d")}));
    // src defect: firstNItems(0) appends first item then checks count >= 0, returns [a]
    EXPECT_EQ(page.firstNItems(0), (QStringList{QStringLiteral("a")}));
}

// --- Additional edge-case coverage (per review) ---

TEST(ItemsPage, FirstNItemsCountExceedsTotal)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    page.appendPage({QStringLiteral("d"), QStringLiteral("e"), QStringLiteral("f")});
    // Request more items than exist → returns all 6
    EXPECT_EQ(page.firstNItems(10), (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"),
                                               QStringLiteral("d"), QStringLiteral("e"), QStringLiteral("f")}));
}

TEST(ItemsPage, InsertItemSpillsToExistingNextPage)
{
    ItemsPage page(2);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b")});
    page.appendPage({QStringLiteral("c"), QStringLiteral("d")});
    // Insert "x" at front of full page0 → "b" spills to page1, "d" spills to new page2
    page.insertItem(QStringLiteral("x"), 0, 0);
    EXPECT_EQ(page.items(0), (QStringList{QStringLiteral("x"), QStringLiteral("a")}));
    EXPECT_EQ(page.items(1), (QStringList{QStringLiteral("b"), QStringLiteral("c")}));
    EXPECT_EQ(page.items(2), (QStringList{QStringLiteral("d")}));
    EXPECT_EQ(page.pageCount(), 3);
}

TEST(ItemsPage, MoveItemSamePageForwardNonAdjacentAppend)
{
    ItemsPage page(3);
    page.appendPage({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    // fromIndex=2 > toIndex=0, not adjacent (2 != 0+1), append → toIndex becomes 1
    page.moveItemPosition(0, 2, 0, 0, true);
    // "c" moved from index 2 to index 1 → [a, c, b]
    EXPECT_EQ(page.items(0), (QStringList{QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("b")}));
}
