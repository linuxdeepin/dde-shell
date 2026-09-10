// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QVariant>

#include "pluginmetadata.h"

using namespace ds;

// Default-constructed metadata is invalid and reports empty identifiers.
TEST(DPluginMetaData, DefaultIsInvalid)
{
    DPluginMetaData meta;
    EXPECT_FALSE(meta.isValid());
    EXPECT_TRUE(meta.pluginId().isEmpty());
    EXPECT_TRUE(meta.pluginDir().isEmpty());
    EXPECT_TRUE(meta.url().isEmpty());
}

// fromJsonString with a well-formed Plugin.Id populates the metadata.
TEST(DPluginMetaData, FromJsonStringValid)
{
    const QByteArray json = R"({"Plugin":{"Id":"org.deepin.ds.test","Url":"main.qml"}})";
    auto meta = DPluginMetaData::fromJsonString(json);
    ASSERT_TRUE(meta.isValid());
    EXPECT_EQ(meta.pluginId(), QStringLiteral("org.deepin.ds.test"));
}

// fromJsonString without an Id yields an invalid metadata.
TEST(DPluginMetaData, FromJsonStringMissingId)
{
    const QByteArray json = R"({"Plugin":{"Url":"main.qml"}})";
    auto meta = DPluginMetaData::fromJsonString(json);
    EXPECT_FALSE(meta.isValid());
    EXPECT_TRUE(meta.pluginId().isEmpty());
}

// fromJsonString with malformed JSON returns invalid metadata and does not throw.
TEST(DPluginMetaData, FromJsonStringMalformed)
{
    const QByteArray json = R"(not a json {{{)";
    EXPECT_NO_THROW({
        auto meta = DPluginMetaData::fromJsonString(json);
        EXPECT_FALSE(meta.isValid());
        EXPECT_TRUE(meta.pluginId().isEmpty());
    });
}

// value() returns stored fields and falls back to the provided default.
TEST(DPluginMetaData, ValueAndDefault)
{
    const QByteArray json = R"({"Plugin":{"Id":"org.test.foo","Url":"bar.qml","Version":"1.2"}})";
    auto meta = DPluginMetaData::fromJsonString(json);
    ASSERT_TRUE(meta.isValid());
    EXPECT_EQ(meta.value("Url").toString(), QStringLiteral("bar.qml"));
    EXPECT_EQ(meta.value("Version").toString(), QStringLiteral("1.2"));
    EXPECT_EQ(meta.value("Missing", QStringLiteral("fallback")).toString(),
              QStringLiteral("fallback"));
}

// value() on invalid metadata always returns the default (short-circuit).
TEST(DPluginMetaData, ValueOnInvalidReturnsDefault)
{
    DPluginMetaData meta;
    EXPECT_EQ(meta.value("Any", 42).toInt(), 42);
    EXPECT_EQ(meta.value("Any").toString(), QString());
}

// rootPluginMetaData is a stable singleton identifying the root plugin.
TEST(DPluginMetaData, RootPluginMetaData)
{
    auto root = DPluginMetaData::rootPluginMetaData();
    ASSERT_TRUE(root.isValid());
    EXPECT_EQ(root.pluginId(), QStringLiteral("org.deepin.ds.root"));
}

// isRootPlugin only matches the canonical root plugin id.
TEST(DPluginMetaData, IsRootPlugin)
{
    EXPECT_TRUE(DPluginMetaData::isRootPlugin(QStringLiteral("org.deepin.ds.root")));
    EXPECT_FALSE(DPluginMetaData::isRootPlugin(QStringLiteral("org.deepin.ds.other")));
}

// url() resolves Url against the plugin directory; absent Url -> empty.
TEST(DPluginMetaData, UrlResolvesAgainstPluginDir)
{
    QTemporaryDir dir(QDir::tempPath() + "/ddestest-XXXXXX");
    ASSERT_TRUE(dir.isValid());
    const QString filePath = dir.path() + "/plugin.json";
    const QString urlRel = QStringLiteral("main.qml");
    {
        QFile f(filePath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write(R"({"Plugin":{"Id":"org.test.url","Url":")" + urlRel.toUtf8() + R"("}})");
        f.close();
    }

    auto meta = DPluginMetaData::fromJsonFile(filePath);
    ASSERT_TRUE(meta.isValid());
    EXPECT_EQ(meta.pluginId(), QStringLiteral("org.test.url"));
    EXPECT_EQ(meta.pluginDir(), QFileInfo(filePath).absoluteDir().path());
    EXPECT_EQ(meta.url(), QDir(meta.pluginDir()).absoluteFilePath(urlRel));
}

// url() is empty when the Url field is absent even on otherwise valid metadata.
TEST(DPluginMetaData, UrlEmptyWhenAbsent)
{
    const QByteArray json = R"({"Plugin":{"Id":"org.test.nourl"}})";
    auto meta = DPluginMetaData::fromJsonString(json);
    ASSERT_TRUE(meta.isValid());
    EXPECT_TRUE(meta.url().isEmpty());
}

// fromJsonFile with a missing path returns invalid metadata (logs a warning).
TEST(DPluginMetaData, FromJsonFileMissing)
{
    auto meta = DPluginMetaData::fromJsonFile(QStringLiteral("/nonexistent/path/to/plugin.json"));
    EXPECT_FALSE(meta.isValid());
    EXPECT_TRUE(meta.pluginId().isEmpty());
    EXPECT_TRUE(meta.pluginDir().isEmpty());
}

// fromJsonFile with a file whose content has no Plugin.Id: open() succeeds but
// fromJsonString() returns invalid metadata, so the `if (!result.isValid())`
// early-return branch in fromJsonFile is taken (no pluginDir is set).
TEST(DPluginMetaData, FromJsonFileInvalidContent)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    // Valid JSON object but the Plugin object carries no Id -> fromJsonString
    // yields invalid metadata, exercising fromJsonFile's invalid-result branch.
    tmp.write(R"({"Plugin":{"Url":"main.qml"}})");
    tmp.close();

    auto meta = DPluginMetaData::fromJsonFile(tmp.fileName());
    EXPECT_FALSE(meta.isValid());
    EXPECT_TRUE(meta.pluginId().isEmpty());
    // pluginDir is only assigned after the validity check passes, so it stays
    // empty when the invalid-result branch is taken.
    EXPECT_TRUE(meta.pluginDir().isEmpty());
}

// fromJsonFile with a real file behaves like fromJsonString plus pluginDir.
TEST(DPluginMetaData, FromJsonFileRoundTrip)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.write(R"({"Plugin":{"Id":"org.test.file"}})");
    tmp.close();

    auto meta = DPluginMetaData::fromJsonFile(tmp.fileName());
    ASSERT_TRUE(meta.isValid());
    EXPECT_EQ(meta.pluginId(), QStringLiteral("org.test.file"));
    EXPECT_FALSE(meta.pluginDir().isEmpty());
}

// Copy construction shares the underlying implicitly-shared data.
TEST(DPluginMetaData, CopyConstructor)
{
    auto meta = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.copy"}})");
    ASSERT_TRUE(meta.isValid());
    DPluginMetaData copy(meta);
    EXPECT_TRUE(copy.isValid());
    EXPECT_EQ(copy.pluginId(), meta.pluginId());
}

// Copy assignment shares the underlying implicitly-shared data.
TEST(DPluginMetaData, CopyAssignment)
{
    auto meta = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.assign"}})");
    DPluginMetaData other;
    other = meta;
    EXPECT_TRUE(other.isValid());
    EXPECT_EQ(other.pluginId(), meta.pluginId());
}

// Move construction transfers ownership of the shared data.
TEST(DPluginMetaData, MoveConstructor)
{
    auto meta = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.move"}})");
    ASSERT_TRUE(meta.isValid());
    DPluginMetaData moved(std::move(meta));
    EXPECT_TRUE(moved.isValid());
    EXPECT_EQ(moved.pluginId(), QStringLiteral("org.test.move"));
}

// Move assignment swaps the underlying shared data pointer.
TEST(DPluginMetaData, MoveAssignment)
{
    auto meta = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.moveassign"}})");
    DPluginMetaData other;
    other = std::move(meta);
    EXPECT_TRUE(other.isValid());
    EXPECT_EQ(other.pluginId(), QStringLiteral("org.test.moveassign"));
}

// operator== compares pluginId only (other fields are irrelevant).
TEST(DPluginMetaData, EqualityByPluginId)
{
    auto a = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.eq"}})");
    auto b = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.eq","Url":"x.qml"}})");
    auto c = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.neq"}})");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

// Destructor must not crash for valid, invalid, moved-from instances.
TEST(DPluginMetaData, DestructorSafety)
{
    EXPECT_NO_THROW({
        DPluginMetaData invalid;
        DPluginMetaData valid = DPluginMetaData::fromJsonString(R"({"Plugin":{"Id":"org.test.dtor"}})");
        DPluginMetaData moved = std::move(valid);
        // invalid, valid (moved-from), moved all go out of scope here.
    });
}
