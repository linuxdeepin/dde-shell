// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockfoldermigrationutils.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QStandardPaths>

namespace dock {
namespace {

TEST(DockFolderMigrationUtils, ResolvesDownloadsPlaceholderAndDeduplicates)
{
    const QString downloadsPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    ASSERT_FALSE(downloadsPath.isEmpty());

    const QStringList resolved = resolveDockedElements({
        QStringLiteral("desktop/dde-file-manager"),
        QStringLiteral("folder/$DOWNLOADS"),
        QStringLiteral("folder/$DOWNLOADS")
    });

    EXPECT_EQ(resolved,
              QStringList({
                  QStringLiteral("desktop/dde-file-manager"),
                  QStringLiteral("folder/%1").arg(downloadsPath)
              }));
}

TEST(DockFolderMigrationUtils, MigratesLegacyDesktopOnlyLayoutWithExtraApps)
{
    const QStringList legacyLayout = {
        QStringLiteral("desktop/dde-file-manager"),
        QStringLiteral("desktop/custom-app"),
        QStringLiteral("desktop/org.deepin.browser"),
    };

    EXPECT_TRUE(shouldMigrateDefaultDockFolders(legacyLayout));

    const QStringList merged = mergedWithDefaultDockFolders(legacyLayout);
    const QStringList defaultFolders = defaultFolderDockedElements();
    ASSERT_EQ(defaultFolders.size(), 2);

    EXPECT_EQ(merged.value(0), QStringLiteral("desktop/dde-file-manager"));
    EXPECT_EQ(merged.value(1), defaultFolders.at(0));
    EXPECT_EQ(merged.value(2), defaultFolders.at(1));
    EXPECT_EQ(merged.value(3), QStringLiteral("desktop/custom-app"));
    EXPECT_EQ(merged.value(4), QStringLiteral("desktop/org.deepin.browser"));
}

TEST(DockFolderMigrationUtils, SkipsLayoutsWithCustomFolderPins)
{
    const QStringList customizedLayout = {
        QStringLiteral("desktop/dde-file-manager"),
        QStringLiteral("folder//tmp"),
        QStringLiteral("desktop/org.deepin.browser"),
    };

    EXPECT_FALSE(shouldMigrateDefaultDockFolders(customizedLayout));
}

TEST(DockFolderMigrationUtils, SkipsLayoutsWithoutFileManagerAnchor)
{
    const QStringList customizedLayout = {
        QStringLiteral("desktop/org.deepin.browser"),
        QStringLiteral("desktop/custom-app"),
    };

    EXPECT_FALSE(shouldMigrateDefaultDockFolders(customizedLayout));
}

}
}
