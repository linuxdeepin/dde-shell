// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "taskmanager.h"
#include "abstractwindowmonitor.h"
#include "taskmanagersettings.h"
#include "appletitem.h"
#include "dockglobalelementmodel.h"
#include "dockitemmodel.h"
#include "foldercontent.h"

#include <QGuiApplication>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QTest>
#include <QStandardItemModel>
#include <QSignalSpy>
#include <QPointer>
#include <QScopedPointer>

class ApplicationFolderModel : public QStandardItemModel
{
    Q_OBJECT
public:
    int contentsQueries = 0;
    QVariantList contents{QVariantMap{{"id", "app"}, {"name", "App"}, {"iconName", "folder"}}};
    ApplicationFolderModel()
    {
        setItemRoleNames({{dock::TaskManager::DesktopIdRole, "desktopId"}, {dock::TaskManager::NameRole, "name"}});
        auto folder = new QStandardItem;
        folder->setData("internal/folders/1", dock::TaskManager::DesktopIdRole);
        folder->setData("Applications", dock::TaskManager::NameRole);
        appendRow(folder);
    }
    Q_INVOKABLE QVariantMap folderInfo(const QString &)
    {
        ++contentsQueries;
        return {{"contents", contents}};
    }
signals:
    void arrangementChanged();
};

class TaskManagerQmlApiTest : public QObject
{
    Q_OBJECT
private slots:
    void cleanup() { dock::TaskManagerSettings::instance()->setDockedElements({}); }

    void providerLibraryIsDiscovered()
    {
        const QString directory = QStringLiteral(TEST_FOLDER_PLUGIN_DIR);
        const QString library = directory + "/org.deepin.ds.folderprovider.testsource.so";
        QVERIFY(QFileInfo::exists(library));
        // Use the actual MODULE filename. An extensionless existence check
        // used to skip the provider before QPluginLoader could load it.
        QVERIFY(!QFileInfo::exists(directory + "/org.deepin.ds.folderprovider.testsource"));
        QCoreApplication::addLibraryPath(directory);
        const QString reference = dock::FolderContentRegistry::reference("testsource", "test://folder");
        QCOMPARE(dock::FolderContentRegistry::instance()->normalizeReference(reference), reference);
        // Missing providers also preserve references. Verify that the plugin
        // actually implements the Dock interface and supplies its presentation.
        QScopedPointer<dock::FolderPresentation> presentation(dock::FolderContentRegistry::instance()->create(reference, nullptr));
        QVERIFY(presentation);
        QCOMPARE(presentation->title(), QStringLiteral("Test folder"));
        QCOMPARE(presentation->previewIcons(), QVariantList{QUrl("image://icon/folder")});
    }

    void appItemComponentCompiles()
    {
        // The backend registers the TextCalculator QML module used by AppItem.
        dock::TaskManager backend;
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(TEST_SHELL_QML_DIR));
        QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(TEST_TASKMANAGER_PACKAGE "/AppItem.qml")));
        QTRY_VERIFY(component.status() != QQmlComponent::Loading);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    }

    void applicationFolderMetadataAndSingleRefresh()
    {
        QStandardItemModel apps, windows;
        ApplicationFolderModel folders;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        global.setApplicationFolderModel(&folders);
        dock::TaskManagerSettings::instance()->setDockedElements({"internal/folders/1"});
        QTRY_COMPARE(global.rowCount(), 1);
        const auto folder = global.index(0, 0);
        QCOMPARE(folder.data(dock::TaskManager::NameRole).toString(), "Applications");
        QCOMPARE(folder.data(dock::TaskManager::MenusRole).toString(), "[]");
        QCOMPARE(folders.contentsQueries, 0);
        QCOMPARE(folder.data(dock::TaskManager::ApplicationFolderContentsRole).toList(), folders.contents);
        QCOMPARE(folders.contentsQueries, 1);

        QSignalSpy changed(&global, &QAbstractItemModel::dataChanged);
        folders.setData(folders.index(0, 0), "Renamed", dock::TaskManager::NameRole);
        folders.contents.clear();
        emit folders.arrangementChanged();
        QCOMPARE(changed.count(), 1);
        const auto roles = changed.first().at(2).value<QList<int>>();
        QVERIFY(roles.contains(dock::TaskManager::NameRole));
        QVERIFY(roles.contains(dock::TaskManager::ApplicationFolderContentsRole));
        QCOMPARE(folder.data(dock::TaskManager::NameRole).toString(), "Renamed");
        QVERIFY(folder.data(dock::TaskManager::ApplicationFolderContentsRole).toList().isEmpty());

        folders.removeRow(0);
        emit folders.arrangementChanged();
        QCOMPARE(global.rowCount(), 0);
        QVERIFY(dock::TaskManagerSettings::instance()->dockedElements().isEmpty());
    }

    void contentPinAndModel()
    {
        QCoreApplication::addLibraryPath(QStringLiteral(TEST_FOLDER_PLUGIN_DIR));
        dock::TaskManager backend;
        auto settings = dock::TaskManagerSettings::instance();
        settings->setDockedElements({});
        QStandardItemModel apps, windows;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        dock::DockItemModel visible(&global);
        const QString reference = dock::FolderContentRegistry::reference("testsource", "test://folder");
        QVERIFY(backend.requestDockByDesktopId(reference));
        QVERIFY(backend.IsDocked(reference));
        QVERIFY(!backend.requestDockByDesktopId(reference));
        QCOMPARE(settings->dockedElements(), QStringList{reference});
        QTRY_COMPARE(visible.rowCount(), 1);
        auto index = visible.index(0, 0);
        QCOMPARE(index.data(dock::TaskManager::ItemIdRole).toString(), reference);
        QCOMPARE(index.data(dock::TaskManager::FolderTypeRole).toString(), QStringLiteral("testsource"));
        QCOMPARE(visible.roleNames().value(dock::TaskManager::ApplicationFolderContentsRole), QByteArray("applicationFolderContents"));
        QVERIFY(index.data(dock::TaskManager::ApplicationFolderContentsRole).toList().isEmpty());
        QPointer<QObject> presentation = index.data(dock::TaskManager::FolderPresentationRole).value<QObject *>();
        QVERIFY(presentation);
        QSignalSpy changed(&visible, &QAbstractItemModel::dataChanged);
        QVERIFY(QMetaObject::invokeMethod(presentation, "rename", Q_ARG(QString, "Updated folder")));
        QTRY_VERIFY(!changed.isEmpty());
        QCOMPARE(index.data(dock::TaskManager::NameRole).toString(), QStringLiteral("Updated folder"));

        // A missing provider must not lose a restored pin or change its ID.
        const QString missing = dock::FolderContentRegistry::reference("unavailable", "other-id");
        settings->setDockedElements({reference, missing});
        QTRY_COMPARE(visible.rowCount(), 2);
        QCOMPARE(visible.index(1, 0).data(dock::TaskManager::FolderTypeRole).toString(), QStringLiteral("unavailable"));
        QVERIFY(visible.index(1, 0).data(dock::TaskManager::FolderPresentationRole).value<QObject *>());
        backend.saveDockElementsOrder({missing, reference, missing});
        QCOMPARE(settings->dockedElements(), (QStringList{missing, reference}));
        QVERIFY(backend.requestUndockByDesktopId(reference));
        QTRY_COMPARE(visible.rowCount(), 1);
        QTRY_VERIFY(presentation.isNull());
        QCOMPARE(visible.index(0, 0).data(dock::TaskManager::ItemIdRole).toString(), missing);
        QVERIFY(backend.requestUndockByDesktopId(missing));
        QTRY_COMPARE(visible.rowCount(), 0);
        QVERIFY(!backend.requestDockByDesktopId("folder/../invalid"));
        QVERIFY(!backend.requestDockByDesktopId(dock::FolderContentRegistry::reference("testsource", "invalid-id")));
    }

    void restoredContentReferences_data()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("id");
        QTest::addColumn<QString>("normalizedId");
        QTest::addColumn<bool>("visible");
        QTest::newRow("valid") << "testsource" << "test://folder" << "test://folder" << true;
        QTest::newRow("rejected") << "testsource" << "invalid-id" << "" << false;
        QTest::newRow("noncanonical") << "testsource" << "test://folder/" << "test://folder" << false;
        QTest::newRow("unavailable") << "unavailable" << "other-id" << "other-id" << true;
    }

    void restoredContentReferences()
    {
        QFETCH(QString, source);
        QFETCH(QString, id);
        QFETCH(QString, normalizedId);
        QFETCH(bool, visible);
        QCoreApplication::addLibraryPath(QStringLiteral(TEST_FOLDER_PLUGIN_DIR));
        const auto reference = dock::FolderContentRegistry::reference(source, id);
        const auto normalized = normalizedId.isEmpty() ? QString()
            : dock::FolderContentRegistry::reference(source, normalizedId);
        QVERIFY(dock::FolderContentRegistry::isReference(reference));
        QCOMPARE(dock::FolderContentRegistry::instance()->normalizeReference(reference), normalized);

        QStandardItemModel apps, windows;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        auto settings = dock::TaskManagerSettings::instance();
        // Restore directly from settings, bypassing the pinning API's validation.
        settings->setDockedElements({reference});
        QCOMPARE(global.rowCount(), visible ? 1 : 0);
        QCOMPARE(settings->dockedElements(), QStringList{reference});
        if (visible) {
            const auto index = global.index(0, 0);
            QCOMPARE(index.data(dock::TaskManager::ItemIdRole).toString(), reference);
            QVERIFY(index.data(dock::TaskManager::FolderPresentationRole).value<QObject *>());
        } else {
            QVERIFY(global.findChildren<dock::FolderPresentation *>().isEmpty());
        }
    }

    void unavailableProviderPin()
    {
        dock::TaskManager backend;
        QStandardItemModel apps, windows;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        const QString reference = dock::FolderContentRegistry::reference("unavailable", "other-id");
        QCOMPARE(dock::FolderContentRegistry::instance()->normalizeReference(reference), reference);
        QVERIFY(backend.requestDockByDesktopId(reference));
        QVERIFY(backend.IsDocked(reference));
        QVERIFY(!backend.requestDockByDesktopId(reference));
        QCOMPARE(dock::TaskManagerSettings::instance()->dockedElements(), QStringList{reference});
        QTRY_COMPARE(global.rowCount(), 1);
        auto object = global.index(0, 0).data(dock::TaskManager::FolderPresentationRole).value<QObject *>();
        QPointer<dock::FolderPresentation> presentation = qobject_cast<dock::FolderPresentation *>(object);
        QVERIFY(presentation);
        QVERIFY(presentation->contentUrl().isEmpty());
        QVERIFY(backend.requestUndockByDesktopId(reference));
        QTRY_COMPARE(global.rowCount(), 0);
        QTRY_VERIFY(presentation.isNull());
    }

    // A rename on disk makes the provider resolve a new id. The persisted pin
    // must be rewritten, otherwise it keeps pointing at a path that no longer
    // exists and the dock entry becomes a dead folder.
    void contentPinFollowsRename()
    {
        QCoreApplication::addLibraryPath(QStringLiteral(TEST_FOLDER_PLUGIN_DIR));
        dock::TaskManager backend;
        auto settings = dock::TaskManagerSettings::instance();
        settings->setDockedElements({});
        QStandardItemModel apps, windows;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        dock::DockItemModel visible(&global);

        const QString reference = dock::FolderContentRegistry::reference("testsource", "test://folder");
        QVERIFY(backend.requestDockByDesktopId(reference));
        QTRY_COMPARE(visible.rowCount(), 1);
        QPointer<QObject> presentation = visible.index(0, 0).data(dock::TaskManager::FolderPresentationRole).value<QObject *>();
        QVERIFY(presentation);

        QVERIFY(QMetaObject::invokeMethod(presentation, "rebind", Q_ARG(QString, "test://folder-renamed")));

        const QString renamed = dock::FolderContentRegistry::reference("testsource", "test://folder-renamed");
        QTRY_COMPARE(settings->dockedElements(), QStringList{renamed});
        QTRY_COMPARE(visible.index(0, 0).data(dock::TaskManager::ItemIdRole).toString(), renamed);
        QCOMPARE(visible.rowCount(), 1);
    }

    // A deleted directory can never show contents again, so the pin has to be
    // dropped instead of leaving a folder entry that stays permanently empty.
    void contentPinDroppedWhenContentLost()
    {
        QCoreApplication::addLibraryPath(QStringLiteral(TEST_FOLDER_PLUGIN_DIR));
        dock::TaskManager backend;
        auto settings = dock::TaskManagerSettings::instance();
        const QString survivor = dock::FolderContentRegistry::reference("unavailable", "keep-me");
        settings->setDockedElements({});
        QStandardItemModel apps, windows;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        dock::DockItemModel visible(&global);

        const QString reference = dock::FolderContentRegistry::reference("testsource", "test://folder");
        // An unavailable provider stands in for temporary unavailability;
        // its pin must survive while the permanently lost pin disappears.
        settings->setDockedElements({survivor});
        QVERIFY(backend.requestDockByDesktopId(reference));
        QTRY_COMPARE(visible.rowCount(), 2);
        QPointer<QObject> presentation;
        for (int row = 0; row < visible.rowCount(); ++row) {
            if (visible.index(row, 0).data(dock::TaskManager::ItemIdRole).toString() == reference)
                presentation = visible.index(row, 0).data(dock::TaskManager::FolderPresentationRole).value<QObject *>();
        }
        QVERIFY(presentation);

        QVERIFY(QMetaObject::invokeMethod(presentation, "lose"));

        // Only the lost pin goes away; an unavailable provider must not be
        // mistaken for a deletion.
        QTRY_COMPARE(settings->dockedElements(), QStringList{survivor});
        QTRY_COMPARE(visible.rowCount(), 1);
        QCOMPARE(visible.index(0, 0).data(dock::TaskManager::ItemIdRole).toString(), survivor);
    }

    void attachedPinningApi()
    {
        // No init()/load(): do not start a dock, window monitor or D-Bus service.
        auto settings = dock::TaskManagerSettings::instance();
        settings->setDockedElements({});
        QStandardItemModel apps, windows;
        ApplicationFolderModel folders;
        dock::DockCombineModel active(&windows, &apps, dock::TaskManager::DesktopIdRole,
            [](QVariant, QAbstractItemModel *) { return QModelIndex(); });
        dock::DockGlobalElementModel global(&apps, &active);
        global.setApplicationFolderModel(&folders);
        dock::TaskManager backend;
        backend.m_dockGlobalElementModel = &global;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("_ds_applet", &backend);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import FolderApiTest 1.0
            QtObject {
                id: root
                function dock(id) { return root.TestApplet.requestDockByDesktopId(id) }
                function undock(id) { return TestApplet.requestUndockByDesktopId(id) }
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        const QString folderId = "internal/folders/1";
        QVariant result;
        // A first pin must succeed through QML, so a later false result cannot
        // be caused by an absent model or an unknown folder.
        QVERIFY(QMetaObject::invokeMethod(object.get(), "dock", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, folderId)));
        QCOMPARE(result, QVariant(true));
        QCOMPARE(settings->dockedElements(), QStringList{folderId});
        QCOMPARE(global.rowCount(), 1);
        QVERIFY(backend.IsDocked(folderId));
        // Already docked: report false so the launcher drop is rejected and the
        // existing pin is not reordered, but never append a duplicate entry.
        QVERIFY(QMetaObject::invokeMethod(object.get(), "dock", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, folderId)));
        QCOMPARE(result, QVariant(false));
        QCOMPARE(settings->dockedElements(), QStringList{folderId});
        QCOMPARE(global.rowCount(), 1);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "undock", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, folderId)));
        QCOMPARE(result, QVariant(true));
        QVERIFY(!settings->isDocked(folderId));
        QVERIFY(!backend.IsDocked(folderId));
        QCOMPARE(global.rowCount(), 0);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "dock", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, folderId)));
        QCOMPARE(result, QVariant(true));
        QCOMPARE(settings->dockedElements(), QStringList{folderId});
        QCOMPARE(global.rowCount(), 1);
        // Reject reserved IDs without consulting the application manager.
        QVERIFY(QMetaObject::invokeMethod(object.get(), "dock", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, "internal/invalid")));
        QCOMPARE(result, QVariant(false));
    }
};

int main(int argc, char **argv)
{
    QTemporaryDir config;
    if (!config.isValid()) return 1;
    // Isolate even constructor-time DConfig migration from the user's desktop.
    qputenv("DSG_DCONFIG_BACKEND_TYPE", "FileBackend");
    qputenv("DSG_DCONFIG_FILE_BACKEND_LOCAL_PREFIX", config.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/folder-api-test-bus");
    QGuiApplication app(argc, argv);
    qmlRegisterUncreatableType<DS_NAMESPACE::DAppletItem>("FolderApiTest", 1, 0, "TestApplet", "Attached applet API only");
    TaskManagerQmlApiTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "taskmanagerqmlapitests.moc"
