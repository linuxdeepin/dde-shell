// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "itemspage.h"
#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>
#include <memory>
#include <QSettings>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>

class MemoryConfig : public Dtk::Core::DConfigBackend
{
public:
    QVariantMap values{{"ArrangementMigrated", true}, {"Groups", QVariantList()}};
    bool isValid() const override { return true; }
    bool load(const QString &) override { return true; }
    QStringList keyList() const override { return values.keys(); }
    QVariant value(const QString &key, const QVariant &fallback) const override { return values.value(key, fallback); }
    void setValue(const QString &key, const QVariant &value) override { values.insert(key, value); }
};

class ReadyApplications : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
public:
    bool ready() const { return m_ready; }
    void setReady(bool ready) { m_ready = ready; emit readyChanged(ready); }
signals:
    void readyChanged(bool ready);
private:
    bool m_ready = true;
};

class AppGroupManagerTest : public QObject
{
    Q_OBJECT
    ReadyApplications applications;
    std::unique_ptr<Dtk::Core::DConfig> configuration;
    std::unique_ptr<apps::AppGroupManager> folders;
    QTemporaryDir configurationHome;
    QByteArray originalConfigHome;

    QString createFolder(const QString &first = "a", const QString &second = "b")
    {
        folders->rearrange(first, second, 0);
        return QStringLiteral("internal/folders/%1").arg(folders->itemPosition(first).value("group").toInt());
    }

private slots:
    void initTestCase()
    {
        QVERIFY(configurationHome.isValid());
        originalConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configurationHome.path().toUtf8());
    }
    void cleanupTestCase()
    {
        if (originalConfigHome.isNull()) qunsetenv("XDG_CONFIG_HOME");
        else qputenv("XDG_CONFIG_HOME", originalConfigHome);
    }
    void init()
    {
        applications.setReady(true);
        for (const auto &id : {"a", "b", "c", "d"}) {
            auto item = new QStandardItem;
            item->setData(QString(id), apps::AppItemModel::DesktopIdRole);
            item->setData(QString(id).toUpper(), apps::AppItemModel::NameRole);
            item->setData(QString(id) + "-icon", apps::AppItemModel::IconNameRole);
            applications.appendRow(item);
        }
        // No DConfig or application-manager service: all mutations are in memory.
        configuration.reset(Dtk::Core::DConfig::create(new MemoryConfig, "folder-tests", "folders"));
        folders = std::make_unique<apps::AppGroupManager>(&applications, configuration.get(), nullptr);
    }
    void cleanup() { folders.reset(); configuration.reset(); applications.clear(); }

    void createAndRename()
    {
        QAbstractItemModelTester tester(folders.get(), QAbstractItemModelTester::FailureReportingMode::QtTest);
        const auto id = createFolder();
        QCOMPARE(id, "internal/folders/1");
        const auto contents = folders->folderInfo(id).value("contents").toList();
        QCOMPARE(contents.size(), 2);
        QCOMPARE(contents[0].toMap().value("id").toString(), "b");
        QCOMPARE(contents[1].toMap().value("id").toString(), "a");
        QSignalSpy changed(folders.get(), &apps::AppGroupManager::arrangementChanged);
        folders->updateFolderName(1, "My apps");
        QCOMPARE(folders->folderInfo(id).value("name").toString(), "My apps");
        QCOMPARE(changed.count(), 1);
    }
    void moveLastChildOut()
    {
        const auto id = createFolder();
        folders->rearrange("b", "c", -1);
        folders->rearrange("a", "c", -1);
        QVERIFY(folders->folderInfo(id).isEmpty());
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"b", "a", "c", "d"}));
    }
    void moveLastChildOntoOwnFolder_data()
    {
        QTest::addColumn<int>("operation");
        QTest::addColumn<int>("layout");
        for (int operation : {-1, 1}) {
            const QByteArray side = operation < 0 ? "prepend" : "append";
            QTest::newRow((side + "-first-page").constData()) << operation << 0;
            QTest::newRow((side + "-only-item-on-last-page").constData()) << operation << 1;
            QTest::newRow((side + "-end-of-full-page").constData()) << operation << 2;
        }
    }
    void moveLastChildOntoOwnFolder()
    {
        QFETCH(int, operation);
        QFETCH(int, layout);
        const auto id = createFolder();
        folders->rearrange("b", "c", -1);
        if (layout == 1) {
            const int page = folders->createEmptyPage(0);
            folders->rearrange(id, "internal/folders/0", 0, page);
        } else if (layout == 2) {
            for (int i = 0; i < 28; ++i) {
                auto item = new QStandardItem;
                item->setData(QStringLiteral("extra-%1").arg(i), apps::AppItemModel::DesktopIdRole);
                applications.appendRow(item);
            }
            QTRY_COMPARE(folders->groupPages(0)->itemCount(), 32);
            folders->rearrange(id, "extra-27", 1);
            QCOMPARE(folders->findItem(id).pos(), 31);
        }
        const auto position = folders->findItem(id);
        auto expected = folders->groupPages(0)->allArrangedItems();
        expected.replace(expected.indexOf(id), "a");

        // Hovering at the source folder's edge can remove it before release.
        folders->rearrange("a.desktop", id, operation);
        QVERIFY(folders->folderInfo(id).isEmpty());
        QCOMPARE(folders->findItem("a").group(), 0);
        QCOMPARE(folders->findItem("a").page(), position.page());
        QCOMPARE(folders->findItem("a").pos(), position.pos());
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), expected);

        // A release still referring to the deleted folder must not lose the app.
        folders->rearrange("a.desktop", id, 0);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), expected);
        folders.reset();
        folders = std::make_unique<apps::AppGroupManager>(&applications, configuration.get(), nullptr);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), expected);
        QCOMPARE(folders->findItem("a").group(), 0);
    }
    void joinIntoOtherFolderAndDeleteSource()
    {
        const auto first = createFolder();
        const auto second = createFolder("c", "d");
        folders->rearrange("a", second, 0);
        folders->rearrange("b", second, 0);
        QVERIFY(folders->folderInfo(first).isEmpty());
        QCOMPARE(folders->folderInfo(second).value("contents").toList().size(), 4);
        // Group identity is not its shifting row number.
        QCOMPARE(folders->groupIndexById(2).data(apps::AppGroupManager::GroupIdRole).toInt(), 2);
        folders->updateFolderName(2, "Remaining folder");
        QCOMPARE(folders->folderInfo(second).value("name").toString(), "Remaining folder");
    }
    void rejectNestedAndStaleDrops()
    {
        const auto first = createFolder();
        const auto second = createFolder("c", "d");
        const auto before = folders->groupPages(0)->allArrangedItems();
        folders->rearrange(first, second, 0);
        folders->rearrange(first, "c", -1);
        folders->rearrange("missing", first, 0);
        folders->rearrange("a", "missing", -1);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), before);
        QCOMPARE(folders->folderInfo(first).value("contents").toList().size(), 2);
    }
    void uninstallRemovesEmptyFolder()
    {
        const auto id = createFolder();
        applications.removeRows(0, 2);
        QTRY_VERIFY(folders->folderInfo(id).isEmpty());
        QVERIFY(!folders->groupPages(0)->contains(id));
        applications.removeRows(0, applications.rowCount());
        QTRY_COMPARE(folders->rowCount(), 1); // Top level survives an empty app set.
    }
    void unreadySourceDoesNotPruneFolders()
    {
        const auto id = createFolder();
        applications.setReady(false);
        applications.removeRows(0, applications.rowCount());
        QCoreApplication::processEvents();
        QCOMPARE(folders->folderInfo(id).value("id").toString(), id);
        QCOMPARE(folders->groupPages(1)->itemCount(), 2);
        // Only the ready notification makes an empty source authoritative.
        applications.setReady(true);
        QTRY_VERIFY(folders->folderInfo(id).isEmpty());
        QCOMPARE(folders->rowCount(), 1);
    }
    void missingSourceKeepsFolderMetadata()
    {
        auto source = std::make_unique<QStandardItemModel>();
        for (const auto &id : {"a", "b"}) {
            auto item = new QStandardItem;
            item->setData(QString(id), apps::AppItemModel::DesktopIdRole);
            source->appendRow(item);
        }
        apps::AppGroupManager manager(source.get(), configuration.get(), nullptr);
        manager.rearrange("a", "b", 0);
        manager.updateFolderName(1, "Still pinned");
        source.reset();
        QCoreApplication::processEvents();
        const auto info = manager.folderInfo("internal/folders/1");
        QCOMPARE(info.value("id").toString(), "internal/folders/1");
        QCOMPARE(info.value("name").toString(), "Still pinned");
        QVERIFY(info.value("contents").toList().isEmpty());
        QCOMPARE(manager.groupPages(1)->itemCount(), 2);
    }
    void bringToFrontAcceptsDesktopSuffix()
    {
        folders->bringToFront("c.desktop");
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"c", "a", "b", "d"}));
        folders->bringToFront("missing.desktop");
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"c", "a", "b", "d"}));
        const auto id = createFolder();
        folders->bringToFront(id);
        QCOMPARE(folders->groupPages(0)->allArrangedItems().first(), id);
    }
    void reorderBeforeAndAfter()
    {
        folders->rearrange("a", "c", -1);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"b", "a", "c", "d"}));
        folders->rearrange("a", "c", 1);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"b", "c", "a", "d"}));
        folders->rearrange("d", "b", 1);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"b", "d", "c", "a"}));
    }
    void persistenceAndLegacyIds()
    {
        auto backend = new MemoryConfig;
        std::unique_ptr<Dtk::Core::DConfig> config(Dtk::Core::DConfig::create(backend, "folder-tests", "folders"));
        backend->values["Groups"] = QVariantList{
            QVariantMap{{"groupId", "internal/group/12"}, {"name", "Imported"},
                        {"appItems", apps::AppGroupManager::fromListOfStringList({{"b.desktop", "a.desktop", "a.desktop"}})}},
            QVariantMap{{"groupId", 0}, {"name", "Top"},
                        {"appItems", apps::AppGroupManager::fromListOfStringList({{"a", "internal/folder/12", "c", "internal/folders/99", "d"}})}}
        };
        folders = std::make_unique<apps::AppGroupManager>(&applications, config.get(), nullptr);
        QCOMPARE(folders->groupPages(0)->allArrangedItems(), QStringList({"internal/folders/12", "c", "d"}));
        QCOMPARE(folders->groupPages(12)->allArrangedItems(), QStringList({"b", "a"}));
        folders->updateFolderName(12, "Persistent folder");
        folders->rearrange("a", "b", -1);
        folders.reset();
        folders = std::make_unique<apps::AppGroupManager>(&applications, config.get(), nullptr);
        QCOMPARE(folders->folderInfo("internal/folders/12").value("name").toString(), "Persistent folder");
        QCOMPARE(folders->groupPages(12)->allArrangedItems(), QStringList({"a", "b"}));
        // Destroy manager before its externally owned config.
        folders.reset();
    }
    void importIniOnlyOnce_data()
    {
        QTest::addColumn<bool>("legacyMigrated");
        QTest::newRow("first-upgrade") << false;
        QTest::newRow("legacy-migration-already-marked") << true;
    }
    void importIniOnlyOnce()
    {
        QFETCH(bool, legacyMigrated);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto originalConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", temporary.path().toUtf8());
        const auto path = temporary.filePath("deepin/dde-launchpad/item-arrangement.ini");
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSettings ini(path, QSettings::NativeFormat);
        ini.setValue("fullscreen/toplevel/pageCount", 2);
        ini.setValue("fullscreen/toplevel/pageItems/0", QStringList({"c.desktop", "internal/folders/5"}));
        ini.setValue("fullscreen/toplevel/pageItems/1", QStringList({"d.desktop", "internal/folders/1"}));
        ini.setValue("fullscreen/1/name", "internal/category/9");
        ini.setValue("fullscreen/1/pageCount", 1);
        ini.setValue("fullscreen/1/pageItems/0", QStringList({"b.desktop"}));
        ini.setValue("fullscreen/5/name", "Legacy folder");
        ini.setValue("fullscreen/5/pageCount", 1);
        ini.setValue("fullscreen/5/pageItems/0", QStringList({"a.desktop"}));
        ini.sync();
        QFile originalIni(path);
        QVERIFY(originalIni.open(QIODevice::ReadOnly));
        const auto iniContents = originalIni.readAll();
        originalIni.close();
        auto backend = new MemoryConfig;
        backend->values["ArrangementMigrated"] = legacyMigrated;
        // Upgrade from an old applet that has already populated the shared key
        // incorrectly. Its migration flag cannot validate the launcher layout.
        backend->values["Groups"] = QVariantList{
            QVariantMap{{"groupId", 1}, {"name", "internal/category/9"},
                        {"appItems", apps::AppGroupManager::fromListOfStringList({{"a", "b", "c", "d"}})}}};
        std::unique_ptr<Dtk::Core::DConfig> config(Dtk::Core::DConfig::create(backend, "folder-tests", "folders"));
        folders = std::make_unique<apps::AppGroupManager>(&applications, config.get(), nullptr);
        // Restore even if any of the checks below fail.
        if (originalConfigHome.isNull()) qunsetenv("XDG_CONFIG_HOME");
        else qputenv("XDG_CONFIG_HOME", originalConfigHome);
        QCOMPARE(folders->folderInfo("internal/folders/5").value("name").toString(), "Legacy folder");
        // The first INI child is the System folder, not the fullscreen root.
        // Keep root apps, folder entrances, page boundaries and order intact.
        const QList<QStringList> topPages{{"c", "internal/folders/5"}, {"d", "internal/folders/1"}};
        QCOMPARE(folders->rowCount(), 3);
        QCOMPARE(folders->groupPages(0)->allPagedItems(), topPages);
        QCOMPARE(folders->groupPages(0)->maxItemCountPerPage(), 32);
        QCOMPARE(folders->groupPages(1)->allArrangedItems(), QStringList({"b"}));
        QCOMPARE(folders->groupPages(5)->allArrangedItems(), QStringList({"a"}));
        QCOMPARE(folders->findItem("c").group(), 0);
        QCOMPARE(folders->findItem("d").group(), 0);
        QVERIFY(!folders->groupPages(0)->contains("internal/folders/0"));
        QVERIFY(QFileInfo::exists(path));
        QVERIFY(originalIni.open(QIODevice::ReadOnly));
        QCOMPARE(originalIni.readAll(), iniContents);
        originalIni.close();
        bool versionedRoot = false;
        for (const auto &value : config->value("Groups").toList()) {
            const auto record = value.toMap();
            if (record.value("groupId").toString() == "internal/folders/0")
                versionedRoot = record.value("arrangementVersion").toInt() == 1;
        }
        QVERIFY(versionedRoot);
        folders->updateFolderName(5, "Renamed after migration");
        folders->rearrange("d", "c", -1);
        const auto updatedPages = folders->groupPages(0)->allPagedItems();
        folders.reset();
        qputenv("XDG_CONFIG_HOME", temporary.path().toUtf8());
        folders = std::make_unique<apps::AppGroupManager>(&applications, config.get(), nullptr);
        if (originalConfigHome.isNull()) qunsetenv("XDG_CONFIG_HOME");
        else qputenv("XDG_CONFIG_HOME", originalConfigHome);
        QCOMPARE(folders->folderInfo("internal/folders/5").value("name").toString(), "Renamed after migration");
        QCOMPARE(folders->groupPages(0)->allPagedItems(), updatedPages);
        QCOMPARE(folders->groupPages(1)->allArrangedItems(), QStringList({"b"}));
        folders.reset();
    }
    void folderCanMoveToNewTopLevelPage()
    {
        const auto id = createFolder();
        QCOMPARE(folders->createEmptyPage(0), 1);
        folders->rearrange(id, "internal/folders/0", 0, 1);
        QCOMPARE(folders->findItem(id).group(), 0);
        QCOMPARE(folders->findItem(id).page(), 1);
        QCOMPARE(folders->folderInfo(id).value("contents").toList().size(), 2);
    }
    void emptyPageAndDesktopSuffix()
    {
        const auto id = createFolder("a.desktop", "b.desktop");
        QCOMPARE(folders->createEmptyPage(1), 1);
        folders->rearrange("a.desktop", id, 0, 1);
        QCOMPARE(folders->findItem("a").page(), 1);
        folders->rearrange("a", "internal/folders/0", 0);
        QCOMPARE(folders->findItem("a").group(), 0);
        folders->removeEmptyPages();
        QCOMPARE(folders->pageCount(1), 1);
    }
};
QTEST_GUILESS_MAIN(AppGroupManagerTest)
#include "appgroupmanagertests.moc"
