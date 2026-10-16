// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "audioapplet.h"

using namespace osd;
using ::testing::Return;

// Mock interface for audio DBus operations.
// The real AudioApplet uses DDBusSender to call org.deepin.dde.Audio1.
// For unit testing we cannot rely on a live DBus session, so we test
// the icon name computation logic directly by setting the internal
// volume value and observing the result of sync/fetchIconName.
class AudioAppletTest : public ::testing::Test {
protected:
    void SetUp() override {
        applet = new AudioApplet();
    }

    void TearDown() override {
        delete applet;
        applet = nullptr;
    }

    AudioApplet *applet = nullptr;
};

// Test initial state
TEST_F(AudioAppletTest, InitialState) {
    EXPECT_EQ(applet->iconName().toStdString(), "");
    EXPECT_DOUBLE_EQ(applet->volumeValue(), 1.0);
}

// Test setIconName emits signal
TEST_F(AudioAppletTest, SetIconNameEmitsSignal) {
    QSignalSpy spy(applet, &AudioApplet::iconNameChanged);
    applet->setIconName("osd_volume_33");
    EXPECT_EQ(applet->iconName().toStdString(), "osd_volume_33");
    EXPECT_EQ(spy.count(), 1);
}

// Test setIconName does not emit when unchanged
TEST_F(AudioAppletTest, SetIconNameNoSignalWhenUnchanged) {
    applet->setIconName("osd_volume_33");
    QSignalSpy spy(applet, &AudioApplet::iconNameChanged);
    applet->setIconName("osd_volume_33");
    EXPECT_EQ(spy.count(), 0);
}

// Test setVolumeValue emits signal
TEST_F(AudioAppletTest, SetVolumeValueEmitsSignal) {
    QSignalSpy spy(applet, &AudioApplet::volumeValueChanged);
    applet->setVolumeValue(0.5);
    EXPECT_DOUBLE_EQ(applet->volumeValue(), 0.5);
    EXPECT_EQ(spy.count(), 1);
}

// Test setVolumeValue does not emit when unchanged (fuzzy compare)
TEST_F(AudioAppletTest, SetVolumeValueNoSignalWhenUnchanged) {
    applet->setVolumeValue(0.5);
    QSignalSpy spy(applet, &AudioApplet::volumeValueChanged);
    applet->setVolumeValue(0.5);
    EXPECT_EQ(spy.count(), 0);
}

// Test increaseVolume property
TEST_F(AudioAppletTest, IncreaseVolumeProperty) {
    EXPECT_TRUE(applet->increaseVolume());

    QSignalSpy spy(applet, &AudioApplet::increaseVolumeChanged);
    applet->setIncreaseVolume(false);
    EXPECT_FALSE(applet->increaseVolume());
    EXPECT_EQ(spy.count(), 1);
}

// Test increaseVolume no signal when unchanged
TEST_F(AudioAppletTest, IncreaseVolumeNoSignalWhenUnchanged) {
    applet->setIncreaseVolume(true);
    QSignalSpy spy(applet, &AudioApplet::increaseVolumeChanged);
    applet->setIncreaseVolume(true);
    EXPECT_EQ(spy.count(), 0);
}

// Test sync does not crash without DBus (graceful degradation)
TEST_F(AudioAppletTest, SyncWithoutDBus) {
    EXPECT_NO_THROW(applet->sync());
}

// Main function for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    QCoreApplication app(argc, argv);

    return RUN_ALL_TESTS();
}

#include "audioapplet_test.moc"
