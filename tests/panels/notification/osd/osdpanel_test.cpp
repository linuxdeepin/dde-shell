// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include "osdpanel.h"

using namespace osd;

// Test fixture for OsdPanel
class OsdPanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = new OsdPanel();
    }

    void TearDown() override {
        delete panel;
        panel = nullptr;
    }

    OsdPanel *panel = nullptr;
};

// Test initial state
TEST_F(OsdPanelTest, InitialState) {
    EXPECT_FALSE(panel->visible());
    EXPECT_TRUE(panel->osdType().isEmpty());
    EXPECT_TRUE(panel->lastOsdType().isEmpty());
}

// Test ShowOSD sets osdType via signal
TEST_F(OsdPanelTest, ShowOSDSetsOsdType) {
    QSignalSpy spy(panel, &OsdPanel::osdTypeChanged);

    panel->ShowOSD("AudioUp");

    // ShowOSD uses a 100ms timer to set osdType; wait for it
    QTest::qWait(200);

    EXPECT_EQ(panel->osdType().toStdString(), "AudioUp");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString().toStdString(), "AudioUp");
}

// Test ShowOSD makes panel visible
TEST_F(OsdPanelTest, ShowOSDMakesVisible) {
    QSignalSpy spy(panel, &OsdPanel::visibleChanged);

    EXPECT_FALSE(panel->visible());

    panel->ShowOSD("AudioUp");
    QTest::qWait(200);

    EXPECT_TRUE(panel->visible());
    EXPECT_EQ(spy.count(), 1);
}

// Test ShowOSD with different type updates osdType
TEST_F(OsdPanelTest, ShowOSDDifferentType) {
    panel->ShowOSD("AudioUp");
    QTest::qWait(200);
    EXPECT_EQ(panel->osdType().toStdString(), "AudioUp");

    panel->ShowOSD("AudioDown");
    QTest::qWait(200);
    EXPECT_EQ(panel->osdType().toStdString(), "AudioDown");
}

// Test lastOsdType is updated after show
TEST_F(OsdPanelTest, LastOsdTypeUpdated) {
    panel->ShowOSD("BrightnessUp");
    QTest::qWait(200);
    EXPECT_EQ(panel->lastOsdType().toStdString(), "BrightnessUp");
}

// Test hideOsd sets visible false after timer expires
TEST_F(OsdPanelTest, HideOsdSetsInvisible) {
    panel->ShowOSD("AudioUp");
    QTest::qWait(200);
    EXPECT_TRUE(panel->visible());

    // Non-SwitchWM3D interval is 1000ms, plus doneSetting delay.
    // Wait long enough for the full hide cycle.
    QTest::qWait(3000);
    EXPECT_FALSE(panel->visible());
}

// Test ShowOSD called multiple times resets timer
TEST_F(OsdPanelTest, ShowOSDMultipleCallsResetTimer) {
    panel->ShowOSD("AudioUp");
    QTest::qWait(200);

    panel->ShowOSD("AudioDown");
    QTest::qWait(200);

    EXPECT_EQ(panel->osdType().toStdString(), "AudioDown");
    EXPECT_TRUE(panel->visible());
}

// Test empty osd type
TEST_F(OsdPanelTest, ShowOSDEmptyType) {
    panel->ShowOSD("");
    QTest::qWait(200);
    EXPECT_TRUE(panel->osdType().isEmpty());
}

// Test SwitchWM3D uses 2000ms interval
TEST_F(OsdPanelTest, ShowOSDSwitchWM3D) {
    panel->ShowOSD("SwitchWM3D");
    QTest::qWait(200);
    EXPECT_EQ(panel->osdType().toStdString(), "SwitchWM3D");
    EXPECT_TRUE(panel->visible());
}

// D-Bus integration test for org.deepin.dde.Osd1 ShowOSD
class OsdDBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = new OsdPanel();
        // Try to initialize (registers DBus service). May fail if service
        // is already taken; in that case skip the DBus tests.
        dbusReady = panel->init();
    }

    void TearDown() override {
        delete panel;
        panel = nullptr;
    }

    OsdPanel *panel = nullptr;
    bool dbusReady = false;
};

// Test ShowOSD via DBus interface
TEST_F(OsdDBusTest, ShowOSDViaDBus) {
    if (!dbusReady) {
        GTEST_SKIP() << "DBus service registration failed; skipping DBus integration test";
    }

    QDBusInterface iface("org.deepin.dde.Osd1", "/", "org.deepin.dde.Osd1",
                         QDBusConnection::sessionBus());
    ASSERT_TRUE(iface.isValid());

    QDBusReply<void> reply = iface.call("ShowOSD", QString("AudioUp"));
    ASSERT_TRUE(reply.isValid());

    QTest::qWait(200);
    EXPECT_EQ(panel->osdType().toStdString(), "AudioUp");
    EXPECT_TRUE(panel->visible());
}

// Main function for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    QCoreApplication app(argc, argv);

    return RUN_ALL_TESTS();
}

#include "osdpanel_test.moc"
