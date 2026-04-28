// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "popupsortutils.h"

#include <gtest/gtest.h>

namespace dock {
namespace {

PopupSortableEntry entry(const QString &name,
                         const QString &typeText = QString(),
                         qint64 size = 0,
                         qint64 modifiedTime = 0,
                         qint64 createdTime = 0,
                         bool directory = false)
{
    PopupSortableEntry value;
    value.name = name;
    value.typeText = typeText;
    value.size = size;
    value.modifiedTime = modifiedTime;
    value.createdTime = createdTime;
    value.directory = directory;
    return value;
}

TEST(PopupSortUtils, CycleStateTogglesCurrentFieldAndResetsNewField)
{
    PopupSortState state;

    state = cyclePopupSortState(state, PopupSortField::Name);
    EXPECT_EQ(state.field, PopupSortField::Name);
    EXPECT_EQ(state.order, Qt::DescendingOrder);

    state = cyclePopupSortState(state, PopupSortField::Type);
    EXPECT_EQ(state.field, PopupSortField::Type);
    EXPECT_EQ(state.order, Qt::AscendingOrder);
}

TEST(PopupSortUtils, SortsByNameAscending)
{
    QList<PopupSortableEntry> entries{
        entry(QStringLiteral("Zoo"), QStringLiteral("z"), 2),
        entry(QStringLiteral("alpha"), QStringLiteral("a"), 1),
        entry(QStringLiteral("Beta"), QStringLiteral("b"), 3),
    };

    sortPopupEntries(&entries, PopupSortState{}, false);

    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries.at(0).name, QStringLiteral("alpha"));
    EXPECT_EQ(entries.at(1).name, QStringLiteral("Beta"));
    EXPECT_EQ(entries.at(2).name, QStringLiteral("Zoo"));
}

TEST(PopupSortUtils, SortsBySizeDescending)
{
    QList<PopupSortableEntry> entries{
        entry(QStringLiteral("first"), QString(), 4),
        entry(QStringLiteral("second"), QString(), 12),
        entry(QStringLiteral("third"), QString(), 8),
    };

    PopupSortState state;
    state.field = PopupSortField::Size;
    state.order = Qt::DescendingOrder;
    sortPopupEntries(&entries, state, false);

    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries.at(0).name, QStringLiteral("second"));
    EXPECT_EQ(entries.at(1).name, QStringLiteral("third"));
    EXPECT_EQ(entries.at(2).name, QStringLiteral("first"));
}

TEST(PopupSortUtils, KeepsDirectoriesFirstWhenRequested)
{
    QList<PopupSortableEntry> entries{
        entry(QStringLiteral("video.mp4"), QString(), 100, 0, 0, false),
        entry(QStringLiteral("Documents"), QString(), 0, 0, 0, true),
        entry(QStringLiteral("Music"), QString(), 0, 0, 0, true),
        entry(QStringLiteral("notes.txt"), QString(), 5, 0, 0, false),
    };

    PopupSortState state;
    state.field = PopupSortField::Size;
    state.order = Qt::DescendingOrder;
    sortPopupEntries(&entries, state, true);

    ASSERT_EQ(entries.size(), 4);
    EXPECT_TRUE(entries.at(0).directory);
    EXPECT_TRUE(entries.at(1).directory);
    EXPECT_FALSE(entries.at(2).directory);
    EXPECT_FALSE(entries.at(3).directory);
}

TEST(PopupSortUtils, SortsByTypeThenName)
{
    QList<PopupSortableEntry> entries{
        entry(QStringLiteral("Painter"), QStringLiteral("Graphics")),
        entry(QStringLiteral("Browser"), QStringLiteral("Internet")),
        entry(QStringLiteral("Mail"), QStringLiteral("Internet")),
    };

    PopupSortState state;
    state.field = PopupSortField::Type;
    sortPopupEntries(&entries, state, false);

    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries.at(0).name, QStringLiteral("Painter"));
    EXPECT_EQ(entries.at(1).name, QStringLiteral("Browser"));
    EXPECT_EQ(entries.at(2).name, QStringLiteral("Mail"));
}

}
}
