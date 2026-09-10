// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QString>

#include "globals.h"

// =================== escapeToObjectPath ===================

TEST(GlobalsEscape, EmptyStringReturnsUnderscore)
{
    EXPECT_EQ(dock::escapeToObjectPath(""), QString("_"));
}

TEST(GlobalsEscape, AlphanumericUnchanged)
{
    EXPECT_EQ(dock::escapeToObjectPath("abc123"), QString("abc123"));
    EXPECT_EQ(dock::escapeToObjectPath("ABC123xyz"), QString("ABC123xyz"));
    EXPECT_EQ(dock::escapeToObjectPath("a"), QString("a"));
    EXPECT_EQ(dock::escapeToObjectPath("0"), QString("0"));
}

TEST(GlobalsEscape, SpaceCharEscaped)
{
    // space (0x20) -> "_20"
    EXPECT_EQ(dock::escapeToObjectPath(" "), QString("_20"));
    EXPECT_EQ(dock::escapeToObjectPath("a b"), QString("a_20b"));
}

TEST(GlobalsEscape, DotCharEscaped)
{
    // '.' (0x2e) -> "_2e"
    EXPECT_EQ(dock::escapeToObjectPath("."), QString("_2e"));
    EXPECT_EQ(dock::escapeToObjectPath("a.b"), QString("a_2eb"));
}

TEST(GlobalsEscape, DashCharEscaped)
{
    // '-' (0x2d) -> "_2d"
    EXPECT_EQ(dock::escapeToObjectPath("-"), QString("_2d"));
    EXPECT_EQ(dock::escapeToObjectPath("a-b"), QString("a_2db"));
}

TEST(GlobalsEscape, UnderscoreEscaped)
{
    // '_' (0x5f) -> "_5f"
    EXPECT_EQ(dock::escapeToObjectPath("_"), QString("_5f"));
}

TEST(GlobalsEscape, MultipleSpecialChars)
{
    EXPECT_EQ(dock::escapeToObjectPath("a b.c-d"),
              QString("a_20b_2ec_2dd"));
}

TEST(GlobalsEscape, SlashesAndColons)
{
    // '/' (0x2f) -> "_2f", ':' (0x3a) -> "_3a"
    EXPECT_EQ(dock::escapeToObjectPath("/"), QString("_2f"));
    EXPECT_EQ(dock::escapeToObjectPath(":"), QString("_3a"));
}

TEST(GlobalsEscape, TypicalDesktopId)
{
    // "org.deepin.Calculator" -> "org_2edeepin_2eCalculator"
    EXPECT_EQ(dock::escapeToObjectPath("org.deepin.Calculator"),
              QString("org_2edeepin_2eCalculator"));
}

// =================== unescapeFromObjectPath ===================

TEST(GlobalsUnescape, EmptyString)
{
    EXPECT_EQ(dock::unescapeFromObjectPath(""), QString(""));
}

TEST(GlobalsUnescape, AlphanumericUnchanged)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("abc123"), QString("abc123"));
}

TEST(GlobalsUnescape, SpaceCharUnescaped)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("_20"), QString(" "));
    EXPECT_EQ(dock::unescapeFromObjectPath("a_20b"), QString("a b"));
}

TEST(GlobalsUnescape, DotCharUnescaped)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("_2e"), QString("."));
    EXPECT_EQ(dock::unescapeFromObjectPath("a_2eb"), QString("a.b"));
}

TEST(GlobalsUnescape, DashCharUnescaped)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("_2d"), QString("-"));
    EXPECT_EQ(dock::unescapeFromObjectPath("a_2db"), QString("a-b"));
}

TEST(GlobalsUnescape, MultipleSpecialChars)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("a_20b_2ec_2dd"),
              QString("a b.c-d"));
}

TEST(GlobalsUnescape, TypicalDesktopId)
{
    EXPECT_EQ(dock::unescapeFromObjectPath("org_2edeepin_2eCalculator"),
              QString("org.deepin.Calculator"));
}

// =================== Round-trip ===================

TEST(GlobalsRoundTrip, RoundTripPreservesString)
{
    QStringList samples = {
        "app1", "org.deepin.Calculator", "a b.c-d",
        "test/path:value", "simple", "X",
        "a_2e", "_20", "no-special",
    };
    for (const auto &s : samples) {
        auto escaped = dock::escapeToObjectPath(s);
        auto unescaped = dock::unescapeFromObjectPath(escaped);
        EXPECT_EQ(unescaped, s) << "round-trip failed for: " << s.toStdString();
    }
}

TEST(GlobalsRoundTrip, DISABLED_UnderscoreStaysLiteralWhenUnescaping)
{
    // A literal '_' not followed by 2 hex chars is left as-is.
    EXPECT_EQ(dock::unescapeFromObjectPath("abc_xyz"), QString("abc_xyz"));
    // A '_' near the end (i+2 >= size) is left as-is.
    EXPECT_EQ(dock::unescapeFromObjectPath("ab_"), QString("ab_"));
    EXPECT_EQ(dock::unescapeFromObjectPath("a_2"), QString("a_2"));
}
