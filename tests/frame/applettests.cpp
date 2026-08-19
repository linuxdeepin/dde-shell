// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for DApplet (frame/applet), the base plugin-instance class.
// Tests the public API: id, pluginId, rootObject/setRootObject (+ signal),
// parentApplet, appletData/setAppletData, pluginMetaData, load, init.
// createProxyMeta is protected — tested via a minimal test subclass.

#include <gtest/gtest.h>

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include "applet.h"
#include "appletdata.h"
#include "pluginmetadata.h"

using namespace ds;

// Test subclass to expose the protected createProxyMeta().
class TestApplet : public DApplet
{
public:
    explicit TestApplet(QObject *parent = nullptr) : DApplet(parent) {}
    using DApplet::createProxyMeta; // lift protected → public for testing
};

// Default-constructed applet has empty id/pluginId and no rootObject.
TEST(DApplet, DefaultState)
{
    DApplet applet;
    EXPECT_TRUE(applet.id().isEmpty());
    EXPECT_TRUE(applet.pluginId().isEmpty());
    EXPECT_EQ(applet.rootObject(), nullptr);
    EXPECT_FALSE(applet.pluginMetaData().isValid());
    EXPECT_EQ(applet.parentApplet(), nullptr);
    EXPECT_FALSE(applet.appletData().isValid());
}

// setAppletData / appletData round-trip; id() reflects the data.
TEST(DApplet, SetGetAppletData)
{
    DApplet applet;
    DAppletData data(QStringLiteral("org.test.plugin"));
    data.setId(QStringLiteral("instance-1"));
    applet.setAppletData(data);

    EXPECT_EQ(applet.appletData().pluginId(), QStringLiteral("org.test.plugin"));
    EXPECT_EQ(applet.appletData().id(), QStringLiteral("instance-1"));
    EXPECT_EQ(applet.id(), QStringLiteral("instance-1"));
}

// setRootObject with a new object emits rootObjectChanged.
TEST(DApplet, SetRootObjectEmitsSignal)
{
    DApplet applet;
    QSignalSpy spy(&applet, &DApplet::rootObjectChanged);
    ASSERT_TRUE(spy.isValid());

    auto *obj = new QObject();
    applet.setRootObject(obj);
    EXPECT_EQ(applet.rootObject(), obj);
    EXPECT_EQ(spy.count(), 1);
}

// setRootObject with the same object does NOT emit the signal.
TEST(DApplet, SetRootObjectSameNoSignal)
{
    DApplet applet;
    auto *obj = new QObject();
    applet.setRootObject(obj);

    QSignalSpy spy(&applet, &DApplet::rootObjectChanged);
    applet.setRootObject(obj); // same — no signal
    EXPECT_EQ(spy.count(), 0);
}

// setRootObject to nullptr emits the signal and clears rootObject.
TEST(DApplet, SetRootObjectNullEmitsSignal)
{
    DApplet applet;
    auto *obj = new QObject();
    applet.setRootObject(obj);

    QSignalSpy spy(&applet, &DApplet::rootObjectChanged);
    applet.setRootObject(nullptr);
    EXPECT_EQ(applet.rootObject(), nullptr);
    EXPECT_EQ(spy.count(), 1);
    delete obj; // clean up; DAppletPrivate destructor would deleteLater it
}

// parentApplet() returns the parent cast to DApplet, or nullptr if not a DApplet.
TEST(DApplet, ParentApplet)
{
    // No parent → null
    {
        DApplet applet;
        EXPECT_EQ(applet.parentApplet(), nullptr);
    }

    // Parent is a DApplet → returns it
    {
        DApplet parent;
        DApplet child(&parent);
        EXPECT_EQ(child.parentApplet(), &parent);
    }

    // Parent is a plain QObject (not DApplet) → nullptr
    {
        QObject plainParent;
        DApplet child(&plainParent);
        EXPECT_EQ(child.parentApplet(), nullptr);
    }
}

// load() and init() return true by default.
TEST(DApplet, LoadInitReturnTrue)
{
    DApplet applet;
    EXPECT_TRUE(applet.load());
    EXPECT_TRUE(applet.init());
}

// createProxyMeta() returns `this` (the default implementation).
TEST(DApplet, CreateProxyMetaReturnsThis)
{
    TestApplet applet;
    EXPECT_EQ(applet.createProxyMeta(), &applet);
}

// Destructor is safe with and without a rootObject set.
// DAppletPrivate destructor calls m_rootObject->deleteLater().
TEST(DApplet, DestructorSafety)
{
    EXPECT_NO_THROW({
        // Without rootObject
        DApplet a1;
        // With rootObject (will be deleteLater'd by DAppletPrivate destructor)
        DApplet a2;
        a2.setRootObject(new QObject());
    });
}
