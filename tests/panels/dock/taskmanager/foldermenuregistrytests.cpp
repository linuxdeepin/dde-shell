// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "foldermenuregistry.h"

#include <QTest>
#include <memory>

class TestFolderMenuProvider : public QObject
{
    Q_OBJECT
public:
    QVariantList actions{QVariantMap{{"id", "details"}, {"name", "Details"}}};
    QString requestedFolder;
    QString triggeredFolder;
    QString triggeredAction;
    int triggerCount = 0;
    Q_INVOKABLE QVariantList folderMenuActions(const QString &folderId)
    {
        requestedFolder = folderId;
        return actions;
    }
    Q_INVOKABLE void triggerFolderMenuAction(const QString &folderId, const QString &actionId)
    {
        triggeredFolder = folderId;
        triggeredAction = actionId;
        ++triggerCount;
    }
};

class FolderMenuRegistryTest : public QObject
{
    Q_OBJECT
private slots:
    void defaultMenuOnlyContainsNameAndUndock()
    {
        dock::FolderMenuRegistry registry;
        const auto items = registry.menuItems("launcher", "internal/folders/12", "My folder");
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].toMap().value("id").toString(), "open-folder");
        QCOMPARE(items[0].toMap().value("name").toString(), "My folder");
        QCOMPARE(items[1].toMap().value("id").toString(), "undock-folder");
        QVERIFY(!registry.trigger("launcher", "internal/folders/12", "details"));
    }
    void customActionsAreAppendedAndDispatchedToTheirSource()
    {
        dock::FolderMenuRegistry registry;
        TestFolderMenuProvider launcher, files;
        QVERIFY(registry.registerProvider("launcher", &launcher));
        QVERIFY(registry.registerProvider("files", &files));
        const auto items = registry.menuItems("launcher", "internal/folders/12", "My folder");
        QCOMPARE(items.size(), 3);
        QCOMPARE(items[2].toMap().value("id").toString(), "details");
        QCOMPARE(launcher.requestedFolder, "internal/folders/12");
        QVERIFY(registry.trigger("launcher", "internal/folders/12", "details"));
        QCOMPARE(launcher.triggeredFolder, "internal/folders/12");
        QCOMPARE(launcher.triggeredAction, "details");
        QCOMPARE(files.triggerCount, 0);
        QCOMPARE(registry.menuItems("unregistered", "id", "Name").size(), 2);
    }
    void reservedDuplicateAndMalformedItemsCannotOverrideTheFramework()
    {
        dock::FolderMenuRegistry registry;
        TestFolderMenuProvider provider;
        provider.actions = {
            QVariantMap{{"id", "open-folder"}, {"name", "Replace open"}},
            QVariantMap{{"id", "undock-folder"}, {"name", "Replace undock"}},
            QVariantMap{{"id", "details"}, {"name", "Details"}},
            QVariantMap{{"id", "details"}, {"name", "Duplicate"}},
            QVariantMap{{"id", ""}, {"name", "No identity"}},
            QVariantMap{{"id", "unnamed"}},
            QVariantMap{{"id", "hidden"}, {"name", "Hidden"}, {"visible", false}}
        };
        QVERIFY(registry.registerProvider("launcher", &provider));
        QCOMPARE(registry.menuItems("launcher", "id", "Folder").size(), 3);
        QVERIFY(!registry.trigger("launcher", "id", "open-folder"));
        QVERIFY(!registry.trigger("launcher", "id", "undock-folder"));
        QVERIFY(!registry.trigger("launcher", "id", "hidden"));
        QVERIFY(!registry.trigger("launcher", "id", "unknown"));
        QCOMPARE(provider.triggerCount, 0);
    }
    void enabledAndCheckedStateIsProvidedByPlugin()
    {
        dock::FolderMenuRegistry registry;
        TestFolderMenuProvider provider;
        provider.actions = {QVariantMap{{"id", "details"}, {"name", "Details"},
                                       {"enabled", false}, {"checkable", true}, {"checked", true}}};
        QVERIFY(registry.registerProvider("launcher", &provider));
        const auto custom = registry.menuItems("launcher", "id", "Folder")[2].toMap();
        QCOMPARE(custom.value("enabled").toBool(), false);
        QCOMPARE(custom.value("checkable").toBool(), true);
        QCOMPARE(custom.value("checked").toBool(), true);
        QVERIFY(!registry.trigger("launcher", "id", "details"));
        provider.actions = {QVariantMap{{"id", "details"}, {"name", "Details"}}};
        QVERIFY(registry.trigger("launcher", "id", "details"));
        // Actions can change while a native menu is open.
        provider.actions.clear();
        QVERIFY(!registry.trigger("launcher", "id", "details"));
        QCOMPARE(provider.triggerCount, 1);
    }
    void registrationIsOwnedByItsPlugin()
    {
        dock::FolderMenuRegistry registry;
        TestFolderMenuProvider first, other;
        QObject invalid;
        QVERIFY(!registry.registerProvider("", &first));
        QVERIFY(!registry.registerProvider("launcher", nullptr));
        QVERIFY(!registry.registerProvider("launcher", &invalid));
        QVERIFY(registry.registerProvider("launcher", &first));
        QVERIFY(registry.registerProvider("launcher", &first));
        QVERIFY(!registry.registerProvider("launcher", &other));
        registry.unregisterProvider("launcher", &other);
        QCOMPARE(registry.menuItems("launcher", "id", "Folder").size(), 3);
        registry.unregisterProvider("launcher", &first);
        QCOMPARE(registry.menuItems("launcher", "id", "Folder").size(), 2);
        QVERIFY(!registry.trigger("launcher", "id", "details"));
        QVERIFY(registry.registerProvider("launcher", &other));
    }
    void destroyingProviderRestoresTheDefaultMenu()
    {
        dock::FolderMenuRegistry registry;
        auto provider = std::make_unique<TestFolderMenuProvider>();
        QVERIFY(registry.registerProvider("launcher", provider.get()));
        provider.reset();
        QCOMPARE(registry.menuItems("launcher", "id", "Folder").size(), 2);
        QVERIFY(!registry.trigger("launcher", "id", "details"));
        TestFolderMenuProvider replacement;
        QVERIFY(registry.registerProvider("launcher", &replacement));
        QCOMPARE(registry.menuItems("launcher", "id", "Folder").size(), 3);
    }
};

QTEST_GUILESS_MAIN(FolderMenuRegistryTest)
#include "foldermenuregistrytests.moc"
