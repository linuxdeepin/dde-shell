// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <initializer_list>
#include <utility>

#include "appletdata.h"
#include "pluginmetadata.h"

using namespace ds;

// Helper: build a QVariantMap from key/value pairs of QStrings.
static QVariantMap makeMap(std::initializer_list<std::pair<const char *, const char *>> items)
{
    QVariantMap map;
    for (const auto &item : items) {
        map[QString::fromLatin1(item.first)] = QString::fromLatin1(item.second);
    }
    return map;
}

// Default-constructed data is invalid (no PluginId).
TEST(DAppletData, DefaultIsInvalid)
{
    DAppletData data;
    EXPECT_FALSE(data.isValid());
    EXPECT_TRUE(data.id().isEmpty());
    EXPECT_TRUE(data.pluginId().isEmpty());
}

// Constructing from a plugin id sets PluginId and makes the data valid.
TEST(DAppletData, ConstructFromPluginId)
{
    DAppletData data(QStringLiteral("org.deepin.ds.test"));
    EXPECT_TRUE(data.isValid());
    EXPECT_EQ(data.pluginId(), QStringLiteral("org.deepin.ds.test"));
    EXPECT_TRUE(data.id().isEmpty()); // Id not set
}

// Constructing from a QVariantMap copies the metadata verbatim.
TEST(DAppletData, ConstructFromVariantMap)
{
    QVariantMap map = makeMap({{"PluginId", "org.deepin.ds.test"}, {"Id", "test-instance"}});
    DAppletData data(map);
    EXPECT_TRUE(data.isValid());
    EXPECT_EQ(data.pluginId(), QStringLiteral("org.deepin.ds.test"));
    EXPECT_EQ(data.id(), QStringLiteral("test-instance"));
}

// setId / id round-trips through the internal metadata.
TEST(DAppletData, SetAndGetId)
{
    DAppletData data(QStringLiteral("org.deepin.ds.test"));
    data.setId(QStringLiteral("my-id"));
    EXPECT_EQ(data.id(), QStringLiteral("my-id"));
}

// value() returns stored keys and falls back to the provided default.
TEST(DAppletData, ValueAndDefault)
{
    QVariantMap map;
    map["PluginId"] = QStringLiteral("org.deepin.ds.test");
    map["Custom"] = 42;
    DAppletData data(map);
    EXPECT_EQ(data.value("Custom").toInt(), 42);
    EXPECT_EQ(data.value("Missing", QStringLiteral("def")).toString(), QStringLiteral("def"));
}

// value() on invalid data always returns the default (short-circuit).
TEST(DAppletData, ValueOnInvalidReturnsDefault)
{
    DAppletData data;
    EXPECT_EQ(data.value("Any", 7).toInt(), 7);
    EXPECT_EQ(data.value("Any").toString(), QString());
}

// toMap() exposes the raw metadata map.
TEST(DAppletData, ToMap)
{
    QVariantMap map;
    map["PluginId"] = QStringLiteral("org.deepin.ds.test");
    map["Id"] = QStringLiteral("inst");
    DAppletData data(map);
    EXPECT_EQ(data.toMap(), map);
}

// groupList on data without a Group key is empty.
TEST(DAppletData, GroupListEmptyByDefault)
{
    DAppletData data(QStringLiteral("org.deepin.ds.test"));
    EXPECT_TRUE(data.groupList().isEmpty());
}

// groupList / setGroupList round-trips nested group metadata.
TEST(DAppletData, GroupListRoundTrip)
{
    DAppletData data(QStringLiteral("org.deepin.ds.test"));

    QVariantMap g1map = makeMap({{"PluginId", "org.deepin.ds.g1"}, {"Id", "g1inst"}});
    QVariantMap g2map = makeMap({{"PluginId", "org.deepin.ds.g2"}, {"Id", "g2inst"}});
    DAppletData g1(g1map);
    DAppletData g2(g2map);
    data.setGroupList({g1, g2});

    const auto groups = data.groupList();
    ASSERT_EQ(groups.size(), 2);
    EXPECT_EQ(groups[0].pluginId(), QStringLiteral("org.deepin.ds.g1"));
    EXPECT_EQ(groups[0].id(), QStringLiteral("g1inst"));
    EXPECT_EQ(groups[1].pluginId(), QStringLiteral("org.deepin.ds.g2"));
    EXPECT_EQ(groups[1].id(), QStringLiteral("g2inst"));
}

// setGroupList with an empty list clears the Group entry.
TEST(DAppletData, SetEmptyGroupList)
{
    DAppletData data(QStringLiteral("org.deepin.ds.test"));
    data.setGroupList({DAppletData(QStringLiteral("org.deepin.ds.g1"))});
    ASSERT_EQ(data.groupList().size(), 1);
    data.setGroupList({});
    EXPECT_TRUE(data.groupList().isEmpty());
}

// fromPluginMetaData copies the plugin id into a new DAppletData.
TEST(DAppletData, FromPluginMetaData)
{
    auto meta = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.deepin.ds.frommeta"}})");
    ASSERT_TRUE(meta.isValid());
    DAppletData data = DAppletData::fromPluginMetaData(meta);
    EXPECT_TRUE(data.isValid());
    EXPECT_EQ(data.pluginId(), meta.pluginId());
}

// fromPluginMetaData on invalid metadata still yields an invalid DAppletData.
TEST(DAppletData, FromInvalidPluginMetaData)
{
    DPluginMetaData invalid;
    DAppletData data = DAppletData::fromPluginMetaData(invalid);
    EXPECT_FALSE(data.isValid());
    EXPECT_TRUE(data.pluginId().isEmpty());
}

// Copy construction shares the implicitly-shared data.
TEST(DAppletData, CopyConstructor)
{
    DAppletData original(QStringLiteral("org.deepin.ds.test"));
    original.setId(QStringLiteral("orig"));
    DAppletData copy(original);
    EXPECT_EQ(copy.pluginId(), original.pluginId());
    EXPECT_EQ(copy.id(), original.id());
}

// Copy assignment shares the implicitly-shared data.
TEST(DAppletData, CopyAssignment)
{
    DAppletData original(QStringLiteral("org.deepin.ds.test"));
    original.setId(QStringLiteral("orig"));
    DAppletData assigned;
    assigned = original;
    EXPECT_EQ(assigned.pluginId(), original.pluginId());
    EXPECT_EQ(assigned.id(), original.id());
}

// operator== compares id only (not pluginId).
TEST(DAppletData, EqualityById)
{
    DAppletData a(QStringLiteral("org.deepin.ds.a"));
    a.setId(QStringLiteral("same-id"));
    DAppletData b(QStringLiteral("org.deepin.ds.b")); // different pluginId
    b.setId(QStringLiteral("same-id"));
    DAppletData c(QStringLiteral("org.deepin.ds.a"));
    c.setId(QStringLiteral("other-id"));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

// Destructor must not crash for default, valid and copied instances.
TEST(DAppletData, DestructorSafety)
{
    EXPECT_NO_THROW({
        DAppletData invalid;
        DAppletData valid(QStringLiteral("org.deepin.ds.dtor"));
        DAppletData copy(valid);
        // all three go out of scope here.
    });
}
