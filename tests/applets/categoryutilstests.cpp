// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for CategoryUtils (applets/dde-apps/categoryutils), a pure
// namespace with three functions that parse desktop-file category strings into
// DDE Categorytype enums. Rich branching: DDE-name match, XDG-name match,
// best-match voting, music+video tie-break, empty input.

#include <gtest/gtest.h>

#include <QList>
#include <QString>
#include <QStringList>

#include "categoryutils.h"

using namespace CategoryUtils;

// --- parseDDECategoryString ---

TEST(ParseDDECategoryString, KnownDDECategories)
{
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("internet")), Categorytype::CategoryInternet);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("chat")), Categorytype::CategoryChat);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("music")), Categorytype::CategoryMusic);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("video")), Categorytype::CategoryVideo);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("graphics")), Categorytype::CategoryGraphics);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("game")), Categorytype::CategoryGame);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("office")), Categorytype::CategoryOffice);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("reading")), Categorytype::CategoryReading);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("development")), Categorytype::CategoryDevelopment);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("system")), Categorytype::CategorySystem);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("others")), Categorytype::CategoryOthers);
}

TEST(ParseDDECategoryString, UnknownReturnsErr)
{
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("nonexistent")), Categorytype::CategoryErr);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("")), Categorytype::CategoryErr);
    EXPECT_EQ(parseDDECategoryString(QStringLiteral("INTERNET")), Categorytype::CategoryErr); // case-sensitive
}

// --- parseXdgCategoryString ---

TEST(ParseXdgCategoryString, KnownXdgCategories)
{
    // Single-match XDG names
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("webbrowser")).size(), 1);
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("webbrowser")).first(), Categorytype::CategoryInternet);
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("ide")).first(), Categorytype::CategoryDevelopment);
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("boardgame")).first(), Categorytype::CategoryGame);
}

TEST(ParseXdgCategoryString, MultiMatchXdgCategories)
{
    // "audiovideo" maps to both Music and Video
    auto result = parseXdgCategoryString(QStringLiteral("audiovideo"));
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains(Categorytype::CategoryMusic));
    EXPECT_TRUE(result.contains(Categorytype::CategoryVideo));

    // "player" maps to both Music and Video
    result = parseXdgCategoryString(QStringLiteral("player"));
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains(Categorytype::CategoryMusic));
    EXPECT_TRUE(result.contains(Categorytype::CategoryVideo));
}

TEST(ParseXdgCategoryString, UnknownReturnsEmpty)
{
    EXPECT_TRUE(parseXdgCategoryString(QStringLiteral("nonexistent")).isEmpty());
    EXPECT_TRUE(parseXdgCategoryString(QStringLiteral("")).isEmpty());
}

TEST(ParseXdgCategoryString, XPrefixCategories)
{
    // x-prefixed categories
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("x-midi")).first(), Categorytype::CategoryMusic);
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("x-bluetooth")).first(), Categorytype::CategorySystem);
    EXPECT_EQ(parseXdgCategoryString(QStringLiteral("x-quran")).first(), Categorytype::CategoryReading);
}

// --- parseBestMatchedCategory ---

TEST(ParseBestMatchedCategory, EmptyInputReturnsOthers)
{
    EXPECT_EQ(parseBestMatchedCategory({}), Categorytype::CategoryOthers);
}

TEST(ParseBestMatchedCategory, SingleDDECategory)
{
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("music")}), Categorytype::CategoryMusic);
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("development")}), Categorytype::CategoryDevelopment);
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("system")}), Categorytype::CategorySystem);
}

TEST(ParseBestMatchedCategory, SingleXdgCategory)
{
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("webbrowser")}), Categorytype::CategoryInternet);
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("ide")}), Categorytype::CategoryDevelopment);
}

TEST(ParseBestMatchedCategory, MultipleSameCategory)
{
    // Multiple categories that all map to the same type
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("music"), QStringLiteral("player")}),
              Categorytype::CategoryMusic);
}

TEST(ParseBestMatchedCategory, VotingPicksMostCommon)
{
    // 2x game + 1x system → game wins
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("game"), QStringLiteral("arcadegame"), QStringLiteral("system")}),
              Categorytype::CategoryGame);
}

TEST(ParseBestMatchedCategory, OnlyOthersReturnsOthers)
{
    // Categories that only map to Others are removed; empty map → Others
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("accessories")}), Categorytype::CategoryOthers);
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("accessories"), QStringLiteral("core")}),
              Categorytype::CategoryOthers);
}

TEST(ParseBestMatchedCategory, MusicVideoTieBreakReturnsVideo)
{
    // Tie between Music and Video → special tie-break returns Video
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("music"), QStringLiteral("video")}),
              Categorytype::CategoryVideo);
}

TEST(ParseBestMatchedCategory, UnknownCategoryIgnored)
{
    // Unknown category strings are ignored; only known ones vote
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("nonexistent"), QStringLiteral("music")}),
              Categorytype::CategoryMusic);
    // All unknown → Others
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("nonexistent1"), QStringLiteral("nonexistent2")}),
              Categorytype::CategoryOthers);
}

TEST(ParseBestMatchedCategory, CaseInsensitiveInput)
{
    // parseBestMatchedCategory calls toLower() on each category
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("MUSIC")}), Categorytype::CategoryMusic);
    EXPECT_EQ(parseBestMatchedCategory({QStringLiteral("Development")}), Categorytype::CategoryDevelopment);
}

TEST(ParseBestMatchedCategory, TieWithoutMusicVideoReturnsFirstSorted)
{
    // Tie between two non-music/video categories → sorted, returns first
    // game + development tie (1 each) → sorted: Development < Game → Development
    auto result = parseBestMatchedCategory({QStringLiteral("game"), QStringLiteral("ide")});
    // Both have 1 vote, sorted: CategoryDevelopment(8) < CategoryGame(5)?
    // Actually enum values: Game=5, Development=8 → sorted ascending: Game < Development
    // So first sorted = Game
    EXPECT_EQ(result, Categorytype::CategoryGame);
}
