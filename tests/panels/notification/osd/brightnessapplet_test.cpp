// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "brightnessapplet.h"

using namespace osd;

// Test fixture for BrightnessApplet
class BrightnessAppletTest : public ::testing::Test {
protected:
    void SetUp() override {
        applet = new BrightnessApplet();
    }

    void TearDown() override {
        delete applet;
        applet = nullptr;
    }

    BrightnessApplet *applet = nullptr;
};

// Test initial state
TEST_F(BrightnessAppletTest, InitialState) {
    EXPECT_DOUBLE_EQ(applet->brightness(), 0.0);
    EXPECT_EQ(applet->iconName().toStdString(), "");
}

// Test sync does not crash without DBus (graceful degradation)
TEST_F(BrightnessAppletTest, SyncWithoutDBus) {
    EXPECT_NO_THROW(applet->sync());
}

// Test sync sets icon name for zero brightness
TEST_F(BrightnessAppletTest, SyncSetsIconForZeroBrightness) {
    applet->sync();
    // Without DBus, fetchBrightness returns 0, so brightness stays 0
    // fetchIconName for brightness <= 0 returns "osd_brightness0"
    EXPECT_EQ(applet->iconName().toStdString(), "osd_brightness0");
}

// Test brightness property signal
TEST_F(BrightnessAppletTest, BrightnessSignalEmission) {
    QSignalSpy spy(applet, &BrightnessApplet::brightnessChanged);
    // sync() with no DBus will call setBrightness(0) which equals initial 0,
    // so no signal is emitted
    applet->sync();
    EXPECT_EQ(spy.count(), 0);
}

// Test iconName signal is emitted on first sync
TEST_F(BrightnessAppletTest, IconNameSignalEmission) {
    QSignalSpy spy(applet, &BrightnessApplet::iconNameChanged);
    applet->sync();
    // iconName changes from "" to "osd_brightness0"
    EXPECT_EQ(spy.count(), 1);
}

// Test iconName does not change when sync called again with same brightness
TEST_F(BrightnessAppletTest, IconNameNoChangeOnRepeatedSync) {
    applet->sync();
    QSignalSpy spy(applet, &BrightnessApplet::iconNameChanged);
    applet->sync();
    // iconName is already "osd_brightness0", no new signal
    EXPECT_EQ(spy.count(), 0);
}

// Main function for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    QCoreApplication app(argc, argv);

    return RUN_ALL_TESTS();
}

#include "brightnessapplet_test.moc"
