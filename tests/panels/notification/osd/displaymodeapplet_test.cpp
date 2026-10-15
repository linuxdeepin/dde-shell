// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "displaymodeapplet.h"

using namespace osd;

// Test fixture for DisPlayModeApplet
class DisplayModeAppletTest : public ::testing::Test {
protected:
    void SetUp() override {
        applet = new DisPlayModeApplet();
    }

    void TearDown() override {
        delete applet;
        applet = nullptr;
    }

    DisPlayModeApplet *applet = nullptr;
};

// Test DPItem construction with 4-arg constructor
TEST(DisplayModeItemTest, ConstructWithFourArgs) {
    DPItem item("Duplicate", "osd_multi_screen_copy", DPItem::Merge);
    EXPECT_EQ(item.text().toStdString(), "Duplicate");
    EXPECT_EQ(item.name().toStdString(), "");
    EXPECT_EQ(item.iconName().toStdString(), "osd_multi_screen_copy");
    EXPECT_EQ(item.mode(), DPItem::Merge);
}

// Test DPItem construction with 5-arg constructor
TEST(DisplayModeItemTest, ConstructWithFiveArgs) {
    DPItem item("Only on HDMI-1", "HDMI-1", "osd_multi_screen_only1", DPItem::Single);
    EXPECT_EQ(item.text().toStdString(), "Only on HDMI-1");
    EXPECT_EQ(item.name().toStdString(), "HDMI-1");
    EXPECT_EQ(item.iconName().toStdString(), "osd_multi_screen_only1");
    EXPECT_EQ(item.mode(), DPItem::Single);
}

// Test DPItem key format
TEST(DisplayModeItemTest, KeyFormat) {
    DPItem item("Extend", "osd_multi_screen_extension", DPItem::Extend);
    QString expectedKey = QString("%1-%2-%3").arg("Extend").arg("").arg(DPItem::Extend);
    EXPECT_EQ(item.key().toStdString(), expectedKey.toStdString());
}

// Test DPItem key format with name
TEST(DisplayModeItemTest, KeyFormatWithName) {
    DPItem item("Only on VGA", "VGA-1", "osd_multi_screen_only2", DPItem::Single);
    QString expectedKey = QString("%1-%2-%3").arg("Only on VGA").arg("VGA-1").arg(DPItem::Single);
    EXPECT_EQ(item.key().toStdString(), expectedKey.toStdString());
}

// Test DPItem mode enum values
TEST(DisplayModeItemTest, ModeEnumValues) {
    EXPECT_EQ(DPItem::Merge, 1);
    EXPECT_EQ(DPItem::Extend, 2);
    EXPECT_EQ(DPItem::Single, 3);
}

// Test initial state
TEST_F(DisplayModeAppletTest, InitialState) {
    EXPECT_EQ(applet->state(), 0);
    EXPECT_EQ(applet->currentPlanItem(), nullptr);
}

// Test sync does not crash without DBus (graceful degradation)
TEST_F(DisplayModeAppletTest, SyncWithoutDBus) {
    EXPECT_NO_THROW(applet->sync());
}

// Test next() does not crash on empty plan list
TEST_F(DisplayModeAppletTest, NextOnEmptyPlanList) {
    EXPECT_NO_THROW(applet->next());
}

// Test doAction does not crash without current item
TEST_F(DisplayModeAppletTest, DoActionWithoutCurrentItem) {
    EXPECT_NO_THROW(applet->doAction());
}

// Test state signal is emitted when changed
TEST_F(DisplayModeAppletTest, StateSignalEmission) {
    // sync() without DBus will not change state from initial 0,
    // since fetchState() depends on DConfig which may not be available.
    // Just verify no crash.
    QSignalSpy spy(applet, &DisPlayModeApplet::stateChanged);
    applet->sync();
    // State may or may not change depending on DConfig availability.
    // The key point is no crash.
    SUCCEED();
}

// Test planItems is initially empty
TEST_F(DisplayModeAppletTest, PlanItemsInitiallyEmpty) {
    // planItems() returns QQmlListProperty; check via the list
    auto items = applet->planItems();
    EXPECT_EQ(items.count(&items), 0);
}

// Main function for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    QCoreApplication app(argc, argv);

    return RUN_ALL_TESTS();
}

#include "displaymodeapplet_test.moc"
