// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QMetaType>

#include "dockiteminfo.h"

// Echo object for DBus round-trip testing of QDBusArgument operators.
class DockItemEchoObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.test.DockItemInfoEcho")
public slots:
    DockItemInfo echo(const DockItemInfo &info) { return info; }
};

// =================== registerPluginInfoMetaType ===================

TEST(DockItemInfo, RegisterPluginInfoMetaTypeRegistersTypes)
{
    registerPluginInfoMetaType();

    int id = QMetaType::fromName("DockItemInfo").id();
    EXPECT_NE(id, QMetaType::UnknownType);

    int listId = QMetaType::fromName("DockItemInfos").id();
    EXPECT_NE(listId, QMetaType::UnknownType);
}

TEST(DockItemInfo, RegisterPluginInfoMetaTypeIdempotent)
{
    registerPluginInfoMetaType();
    registerPluginInfoMetaType();

    EXPECT_NE(QMetaType::fromName("DockItemInfo").id(), QMetaType::UnknownType);
    SUCCEED();
}

// =================== Struct field access ===================

TEST(DockItemInfo, FieldAccessAndAssignment)
{
    DockItemInfo info;
    info.name = "calculator";
    info.displayName = "Calculator";
    info.itemKey = "item-calc";
    info.settingKey = "setting-calc";
    info.dccIcon = "icon-calc";
    info.visible = true;

    EXPECT_EQ(info.name, "calculator");
    EXPECT_EQ(info.displayName, "Calculator");
    EXPECT_EQ(info.itemKey, "item-calc");
    EXPECT_EQ(info.settingKey, "setting-calc");
    EXPECT_EQ(info.dccIcon, "icon-calc");
    EXPECT_TRUE(info.visible);
}

TEST(DockItemInfo, VisibleFalse)
{
    DockItemInfo info;
    info.name = "hidden";
    info.visible = false;
    EXPECT_FALSE(info.visible);
}

TEST(DockItemInfo, CopySemantics)
{
    DockItemInfo original;
    original.name = "app1";
    original.displayName = "App 1";
    original.itemKey = "key1";
    original.settingKey = "set1";
    original.dccIcon = "icon1";
    original.visible = false;

    DockItemInfo copy = original;
    EXPECT_EQ(copy.name, original.name);
    EXPECT_EQ(copy.displayName, original.displayName);
    EXPECT_EQ(copy.itemKey, original.itemKey);
    EXPECT_EQ(copy.settingKey, original.settingKey);
    EXPECT_EQ(copy.dccIcon, original.dccIcon);
    EXPECT_EQ(copy.visible, original.visible);
}

TEST(DockItemInfo, EmptyFields)
{
    DockItemInfo info;
    info.name = "";
    info.displayName = "";
    info.itemKey = "";
    info.settingKey = "";
    info.dccIcon = "";
    info.visible = false;

    QString output;
    QDebug debug(&output);
    debug << info;

    EXPECT_TRUE(output.contains("name:"));
    EXPECT_TRUE(output.contains("visible: false"));
}

// =================== QDebug operator<< ===================

TEST(DockItemInfo, QDebugOperatorContainsAllFields)
{
    DockItemInfo info;
    info.name = "testapp";
    info.displayName = "Test App";
    info.itemKey = "key123";
    info.settingKey = "setting456";
    info.dccIcon = "dcc_icon";
    info.visible = true;

    QString output;
    QDebug debug(&output);
    debug << info;

    EXPECT_TRUE(output.contains("testapp"));
    EXPECT_TRUE(output.contains("Test App"));
    EXPECT_TRUE(output.contains("key123"));
    EXPECT_TRUE(output.contains("setting456"));
    EXPECT_TRUE(output.contains("dcc_icon"));
    EXPECT_TRUE(output.contains("true"));
}

TEST(DockItemInfo, QDebugOperatorVisibleFalse)
{
    DockItemInfo info;
    info.name = "hidden";
    info.visible = false;

    QString output;
    QDebug debug(&output);
    debug << info;

    EXPECT_TRUE(output.contains("false"));
}

TEST(DockItemInfo, QDebugOperatorSpecialCharacters)
{
    DockItemInfo info;
    info.name = "org.deepin.app";
    info.displayName = "中文测试";
    info.itemKey = "key with spaces";
    info.settingKey = "setting/special";
    info.dccIcon = "icon@2x";
    info.visible = true;

    QString output;
    QDebug debug(&output);
    debug << info;

    EXPECT_TRUE(output.contains("org.deepin.app"));
    EXPECT_TRUE(output.contains("中文测试"));
    EXPECT_TRUE(output.contains("key with spaces"));
    EXPECT_TRUE(output.contains("icon@2x"));
}

// =================== QDBusArgument round-trip (operator<< + operator>>) ===================

TEST(DockItemInfo, QDBusArgumentRoundTripViaSessionBus)
{
    registerPluginInfoMetaType();

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        GTEST_SKIP() << "D-Bus session bus not available, skipping round-trip test";
    }

    static int s_counter = 0;
    QString serviceName = QStringLiteral("org.test.DockItemInfo_%1_%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++s_counter);

    ASSERT_TRUE(bus.registerService(serviceName));

    DockItemEchoObject echoObj;
    ASSERT_TRUE(bus.registerObject("/echo", &echoObj,
        QDBusConnection::ExportAllSlots));

    DockItemInfo info;
    info.name = "roundtrip-app";
    info.displayName = "Round Trip App";
    info.itemKey = "rt-key";
    info.settingKey = "rt-setting";
    info.dccIcon = "rt-icon";
    info.visible = true;

    QDBusInterface iface(serviceName, "/echo", "org.test.DockItemInfoEcho", bus);
    ASSERT_TRUE(iface.isValid());

    QDBusReply<DockItemInfo> reply = iface.call("echo", QVariant::fromValue(info));
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    DockItemInfo result = reply.value();
    EXPECT_EQ(result.name, info.name);
    EXPECT_EQ(result.displayName, info.displayName);
    EXPECT_EQ(result.itemKey, info.itemKey);
    EXPECT_EQ(result.settingKey, info.settingKey);
    EXPECT_EQ(result.dccIcon, info.dccIcon);
    EXPECT_EQ(result.visible, info.visible);

    bus.unregisterObject("/echo");
    bus.unregisterService(serviceName);
}

TEST(DockItemInfo, QDBusArgumentRoundTripVisibleFalse)
{
    registerPluginInfoMetaType();

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        GTEST_SKIP() << "D-Bus session bus not available, skipping round-trip test";
    }

    static int s_counter = 0;
    QString serviceName = QStringLiteral("org.test.DockItemInfo2_%1_%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++s_counter);

    ASSERT_TRUE(bus.registerService(serviceName));

    DockItemEchoObject echoObj;
    ASSERT_TRUE(bus.registerObject("/echo", &echoObj,
        QDBusConnection::ExportAllSlots));

    DockItemInfo info;
    info.name = "hidden-app";
    info.displayName = "Hidden";
    info.itemKey = "h-key";
    info.settingKey = "h-set";
    info.dccIcon = "h-icon";
    info.visible = false;

    QDBusInterface iface(serviceName, "/echo", "org.test.DockItemInfoEcho", bus);
    ASSERT_TRUE(iface.isValid());

    QDBusReply<DockItemInfo> reply = iface.call("echo", QVariant::fromValue(info));
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    DockItemInfo result = reply.value();
    EXPECT_EQ(result.name, info.name);
    EXPECT_EQ(result.visible, false);

    bus.unregisterObject("/echo");
    bus.unregisterService(serviceName);
}

#include "dockiteminfotests.moc"
