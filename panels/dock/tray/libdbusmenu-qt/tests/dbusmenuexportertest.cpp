/* This file is part of the dbusmenu-qt library
   Copyright 2009 Canonical
   Author: Aurelien Gateau <aurelien.gateau@canonical.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License (LGPL) as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
// Self
#include "dbusmenuexportertest.h"

// Qt
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QIcon>
#include <QMenu>
#include <QtTest>

// DBusMenuQt
#include <dbusmenuexporter.h>
#include <dbusmenutypes_p.h>
#include <dbusmenushortcut_p.h>
#include <debug_p.h>

// Local
#include "testutils.h"

QTEST_MAIN(DBusMenuExporterTest)

static const char *TEST_SERVICE = "org.kde.dbusmenu-qt-test";
static const char *TEST_OBJECT_PATH = "/TestMenuBar";

Q_DECLARE_METATYPE(QList<int>)

static DBusMenuLayoutItemList getChildren(QDBusAbstractInterface* iface, int parentId, const QStringList &propertyNames)
{
    QDBusPendingReply<uint, DBusMenuLayoutItem> reply = iface->call("GetLayout", parentId, /*recursionDepth=*/ 1, propertyNames);
    reply.waitForFinished();
    if (!reply.isValid()) {
        qFatal("%s", qPrintable(reply.error().message()));
        return DBusMenuLayoutItemList();
    }

    DBusMenuLayoutItem rootItem = reply.argumentAt<1>();
    return rootItem.children;
}

void DBusMenuExporterTest::init()
{
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus, false);
}

void DBusMenuExporterTest::cleanup()
{
    QVERIFY(QDBusConnection::sessionBus().unregisterService(TEST_SERVICE));
}

void DBusMenuExporterTest::testGetSomeProperties_data()
{
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("iconName");
    QTest::addColumn<bool>("enabled");

    QTest::newRow("label only")           << "label" << QString()   << true;
    QTest::newRow("disabled, label only") << "label" << QString()   << false;
    QTest::newRow("icon name")            << "label" << "edit-undo" << true;
}

void DBusMenuExporterTest::testGetSomeProperties()
{
    QFETCH(QString, label);
    QFETCH(QString, iconName);
    QFETCH(bool, enabled);

    // Create an exporter for a menu with one action, defined by the test data
    QMenu inputMenu;
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *action = new QAction(label, &inputMenu);
    if (!iconName.isEmpty()) {
        QIcon icon = QIcon::fromTheme(iconName);
        QVERIFY(!icon.isNull());
        action->setIcon(icon);
    }
    action->setEnabled(enabled);
    inputMenu.addAction(action);

    // Check out exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get exported menu info
    QStringList propertyNames = QStringList() << "type" << "enabled" << "label" << "icon-name";
    DBusMenuLayoutItemList list = getChildren(&iface, /*parentId=*/0, propertyNames);
    DBusMenuLayoutItem item = list.first();
    QVERIFY(item.id != 0);
    QVERIFY(item.children.isEmpty());
    QVERIFY(!item.properties.contains("type"));
    QCOMPARE(item.properties.value("label").toString(), label);
    if (enabled) {
        QVERIFY(!item.properties.contains("enabled"));
    } else {
        QCOMPARE(item.properties.value("enabled").toBool(), false);
    }
    if (iconName.isEmpty()) {
        QVERIFY(!item.properties.contains("icon-name"));
    } else {
        QCOMPARE(item.properties.value("icon-name").toString(), iconName);
    }
}

void DBusMenuExporterTest::testGetAllProperties()
{
    // set of properties which must be returned because their values are not
    // the default values
    QStringList a1Properties = QStringList()
        << "label"
        ;
    a1Properties.sort();

    QStringList separatorProperties = QStringList()
        << "type";
    separatorProperties.sort();

    QStringList a2Properties = QStringList()
        << "label"
        << "enabled"
        << "icon-name"
        << "icon-data" // Icon data is always provided if the icon is valid.
        << "visible"
        ;
    a2Properties.sort();

    // Create the menu items
    QMenu inputMenu;
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    inputMenu.addAction("a1");

    inputMenu.addSeparator();

    QAction *a2 = new QAction("a2", &inputMenu);
    a2->setEnabled(false);
    QIcon icon = QIcon::fromTheme("edit-undo");
    QVERIFY(!icon.isNull());
    a2->setIcon(icon);
    a2->setVisible(false);
    inputMenu.addAction(a2);

    // Export them
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get children
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 3);

    // Check we get the right properties
    DBusMenuLayoutItem item = list.takeFirst();
    auto pKeys = item.properties.keys();
    pKeys.sort();
    QCOMPARE(pKeys, a1Properties);

    item = list.takeFirst();
    pKeys = item.properties.keys();
    pKeys.sort();
    QCOMPARE(pKeys, separatorProperties);

    item = list.takeFirst();
    pKeys = item.properties.keys();
    pKeys.sort();
    QCOMPARE(pKeys, a2Properties);
}

void DBusMenuExporterTest::testGetNonExistentProperty()
{
    const char* NON_EXISTENT_KEY = "i-do-not-exist";

    QMenu inputMenu;
    inputMenu.addAction("a1");
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList() << NON_EXISTENT_KEY);
    QCOMPARE(list.count(), 1);

    DBusMenuLayoutItem item = list.takeFirst();
    QVERIFY(!item.properties.contains(NON_EXISTENT_KEY));
}

void DBusMenuExporterTest::testClickedEvent()
{
    QMenu inputMenu;
    QAction *action = inputMenu.addAction("a1");
    QSignalSpy spy(action, SIGNAL(triggered()));
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 1);
    int id = list.first().id;

    QVariant empty = QVariant::fromValue(QDBusVariant(QString()));
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    iface.call("Event", id, "clicked", empty, timestamp);
    QTest::qWait(500);

    QCOMPARE(spy.count(), 1);
}

void DBusMenuExporterTest::testSubMenu()
{
    QMenu inputMenu;
    QMenu *subMenu = inputMenu.addMenu("menu");
    QAction *a1 = subMenu->addAction("a1");
    QAction *a2 = subMenu->addAction("a2");
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 1);
    int id = list.first().id;

    list = getChildren(&iface, id, QStringList());
    QCOMPARE(list.count(), 2);

    DBusMenuLayoutItem item = list.takeFirst();
    QVERIFY(item.id != 0);
    QCOMPARE(item.properties.value("label").toString(), a1->text());

    item = list.takeFirst();
    QCOMPARE(item.properties.value("label").toString(), a2->text());
}

void DBusMenuExporterTest::testDynamicSubMenu()
{
    // Track LayoutUpdated() signal: we don't want this signal to be emitted
    // too often because it causes refreshes
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    ManualSignalSpy layoutUpdatedSpy;
    QDBusConnection::sessionBus().connect(TEST_SERVICE, TEST_OBJECT_PATH, "com.canonical.dbusmenu", "LayoutUpdated", "ui", &layoutUpdatedSpy, SLOT(receiveCall(uint, int)));

    // Create our test menu
    QMenu inputMenu;
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);
    QAction *action = inputMenu.addAction("menu");
    QMenu *subMenu = new QMenu(&inputMenu);
    action->setMenu(subMenu);
    MenuFiller filler(subMenu);
    filler.addAction(new QAction("a1", subMenu));
    filler.addAction(new QAction("a2", subMenu));

    // Get id of submenu
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 1);
    int id = list.first().id;

    // Nothing for now
    QCOMPARE(subMenu->actions().count(), 0);

    // LayoutUpdated should be emitted once because inputMenu is filled
    QTest::qWait(500);
    QCOMPARE(layoutUpdatedSpy.count(), 1);
    QCOMPARE(layoutUpdatedSpy.takeFirst().at(1).toInt(), 0);

    // Pretend we show the menu
    QDBusReply<bool> aboutToShowReply = iface.call("AboutToShow", id);
    QVERIFY2(aboutToShowReply.isValid(), qPrintable(aboutToShowReply.error().message()));
    QVERIFY(aboutToShowReply.value());
    QTest::qWait(500);
    QCOMPARE(layoutUpdatedSpy.count(), 1);
    QCOMPARE(layoutUpdatedSpy.takeFirst().at(1).toInt(), id);

    // Get submenu items
    list = getChildren(&iface, id, QStringList());
    QVERIFY(subMenu->actions().count() > 0);
    QCOMPARE(list.count(), subMenu->actions().count());

    for (int pos=0; pos< list.count(); ++pos) {
        DBusMenuLayoutItem item = list.at(pos);
        QVERIFY(item.id != 0);
        QAction *action = subMenu->actions().at(pos);
        QVERIFY(action);
        QCOMPARE(item.properties.value("label").toString(), action->text());
    }
}

void DBusMenuExporterTest::testRadioItems()
{
    DBusMenuLayoutItem item;
    DBusMenuLayoutItemList list;
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    // Create 2 radio items, check first one
    QAction *a1 = inputMenu.addAction("a1");
    a1->setCheckable(true);
    QAction *a2 = inputMenu.addAction("a1");
    a2->setCheckable(true);

    QActionGroup group(0);
    group.addAction(a1);
    group.addAction(a2);
    a1->setChecked(true);

    QVERIFY(!a2->isChecked());

    // Get item ids
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 2);

    // Check items are radios and correctly toggled
    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-type").toString(), QString("radio"));
    QCOMPARE(item.properties.value("toggle-state").toInt(), 1);
    int a1Id = item.id;
    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-type").toString(), QString("radio"));
    QCOMPARE(item.properties.value("toggle-state").toInt(), 0);
    int a2Id = item.id;

    // Click a2
    ManualSignalSpy spy;
    QDBusConnection::sessionBus().connect(TEST_SERVICE, TEST_OBJECT_PATH, "com.canonical.dbusmenu", "ItemsPropertiesUpdated", "a(ia{sv})a(ias)",
        &spy, SLOT(receiveCall(DBusMenuItemList, DBusMenuItemKeysList)));
    QVariant empty = QVariant::fromValue(QDBusVariant(QString()));
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    iface.call("Event", a2Id, "clicked", empty, timestamp);
    QTest::qWait(500);

    // Check a1 is not checked, but a2 is
    list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 2);

    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-state").toInt(), 0);

    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-state").toInt(), 1);

    // Did we get notified?
    QCOMPARE(spy.count(), 1);
    QSet<int> updatedIds;
    {
        QVariantList lst = spy.takeFirst().at(0).toList();
        Q_FOREACH(QVariant variant, lst) {
            updatedIds << variant.toInt();
        }
    }

    QSet<int> expectedIds;
    expectedIds << a1Id << a2Id;

    QCOMPARE(updatedIds, expectedIds);
}

void DBusMenuExporterTest::testNonExclusiveActionGroup()
{
    DBusMenuLayoutItem item;
    DBusMenuLayoutItemList list;
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    // Create 2 checkable items
    QAction *a1 = inputMenu.addAction("a1");
    a1->setCheckable(true);
    QAction *a2 = inputMenu.addAction("a1");
    a2->setCheckable(true);

    // Put them into a non exclusive group
    QActionGroup group(0);
    group.addAction(a1);
    group.addAction(a2);
    group.setExclusive(false);

    // Get item ids
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 2);

    // Check items are checkmark, not radio
    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-type").toString(), QString("checkmark"));
    int a1Id = item.id;
    item = list.takeFirst();
    QCOMPARE(item.properties.value("toggle-type").toString(), QString("checkmark"));
    int a2Id = item.id;
}

void DBusMenuExporterTest::testClickDeletedAction()
{
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");

    // Get id
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), 1);
    int id = list.takeFirst().id;

    // Delete a1, it should not cause a crash when trying to trigger it
    delete a1;

    // Send a click to deleted a1
    QVariant empty = QVariant::fromValue(QDBusVariant(QString()));
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    iface.call("Event", id, "clicked", empty, timestamp);
    QTest::qWait(500);
}

// Reproduce LP BUG 521011
// https://bugs.launchpad.net/bugs/521011
void DBusMenuExporterTest::testDeleteExporterBeforeMenu()
{
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");
    delete exporter;
    inputMenu.removeAction(a1);
}

void DBusMenuExporterTest::testUpdateAndDeleteSubMenu()
{
    // Create a menu with a submenu
    QMenu inputMenu;
    QMenu *subMenu = inputMenu.addMenu("menu");
    QAction *a1 = subMenu->addAction("a1");

    // Export it
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    // Update a1 (which is in subMenu) and delete subMenu right after that. If
    // DBusMenuExporter is not careful it will crash in the qWait() because it
    // tries to send itemUpdated() for a1.
    a1->setText("Not a menu anymore");
    delete subMenu;
    QTest::qWait(500);
}

void DBusMenuExporterTest::testMenuShortcut()
{
    // Create a menu containing an action with a shortcut
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");
    a1->setShortcut(Qt::CTRL | Qt::Key_A);

    QAction *a2 = inputMenu.addAction("a2");
    a2->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A, Qt::ALT | Qt::Key_B));

    // No shortcut, to test the property is not added in this case
    QAction *a3 = inputMenu.addAction("a3");

    QList<QAction*> actionList;
    actionList << a1 << a2 << a3;

    // Check out exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get exported menu info
    QStringList propertyNames = QStringList() << "label" << "shortcut";
    DBusMenuLayoutItemList list = getChildren(&iface, 0, propertyNames);
    QCOMPARE(list.count(), actionList.count());

    Q_FOREACH(const QAction* action, actionList) {
        DBusMenuLayoutItem item = list.takeFirst();
        if (action->shortcut().isEmpty()) {
            QVERIFY(!item.properties.contains("shortcut"));
        } else {
            QVERIFY(item.properties.contains("shortcut"));
            QDBusArgument arg = item.properties.value("shortcut").value<QDBusArgument>();
            DBusMenuShortcut shortcut;
            arg >> shortcut;
            QCOMPARE(shortcut.toKeySequence(), action->shortcut());
        }
    }
}

void DBusMenuExporterTest::testGetGroupProperties()
{
    // Create a menu containing two actions
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");
    QAction *a2 = inputMenu.addAction("a2");

    // Check exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get item ids
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), inputMenu.actions().count());

    int id1 = list.at(0).id;
    int id2 = list.at(1).id;

    // Get group properties
    QList<int> ids = QList<int>() << id1 << id2;
    QDBusReply<DBusMenuItemList> reply = iface.call("GetGroupProperties", QVariant::fromValue(ids), QStringList());
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
    DBusMenuItemList groupPropertiesList = reply.value();

    // Check the info we received
    QCOMPARE(groupPropertiesList.count(), inputMenu.actions().count());

    Q_FOREACH(const QAction* action, inputMenu.actions()) {
        DBusMenuItem item = groupPropertiesList.takeFirst();
        QCOMPARE(item.properties.value("label").toString(), action->text());
    }
}

void DBusMenuExporterTest::testActivateAction()
{
    // Create a menu containing two actions
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");
    QAction *a2 = inputMenu.addAction("a2");

    // Check exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    ManualSignalSpy spy;
    QDBusConnection::sessionBus().connect(TEST_SERVICE, TEST_OBJECT_PATH, "com.canonical.dbusmenu", "ItemActivationRequested", "iu", &spy, SLOT(receiveCall(int, uint)));

    // Get item ids
    DBusMenuLayoutItemList list = getChildren(&iface, 0, QStringList());
    QCOMPARE(list.count(), inputMenu.actions().count());

    int id1 = list.at(0).id;
    int id2 = list.at(1).id;

    // Trigger actions
    exporter->activateAction(a1);
    exporter->activateAction(a2);

    // Check we received the signals in the correct order
    QTest::qWait(500);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.takeFirst().at(0).toInt(), id1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), id2);
}

static int trackCount(QMenu* menu)
{
    QList<QObject*> lst = menu->findChildren<QObject*>();
    int count = 0;
    Q_FOREACH(QObject* child, lst) {
        if (qstrcmp(child->metaObject()->className(), "DBusMenu") == 0) {
            ++count;
        }
    }
    return count;
}

// Check we do not create more than one DBusMenu object for each menu
// See KDE bug 254066
void DBusMenuExporterTest::testTrackActionsOnlyOnce()
{
    // Create a menu with a submenu, unplug the submenu and plug it back. The
    // submenu should not have more than one DBusMenu child object.
    QMenu mainMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &mainMenu);

    QMenu* subMenu = new QMenu("File");
    subMenu->addAction("a1");
    mainMenu.addAction(subMenu->menuAction());

    QTest::qWait(500);
    QCOMPARE(trackCount(subMenu), 1);

    mainMenu.removeAction(subMenu->menuAction());

    mainMenu.addAction(subMenu->menuAction());

    QTest::qWait(500);
    QCOMPARE(trackCount(subMenu), 1);
}

// If desktop does not want icon in menus, check we do not export them
void DBusMenuExporterTest::testHonorDontShowIconsInMenusAttribute()
{
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus, true);
    QMenu inputMenu;
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *action = new QAction("Undo", &inputMenu);
    QIcon icon = QIcon::fromTheme("edit-undo");
    QVERIFY(!icon.isNull());
    action->setIcon(icon);
    inputMenu.addAction(action);

    // Check out exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get exported menu info
    QStringList propertyNames = QStringList() << "icon-name";
    DBusMenuLayoutItemList list = getChildren(&iface, /*parentId=*/0, propertyNames);
    DBusMenuLayoutItem item = list.first();
    QVERIFY(item.id != 0);
    QVERIFY(!item.properties.contains("icon-name"));
}

static bool hasInternalDBusMenuObject(QMenu* menu)
{
    Q_FOREACH(QObject* obj, menu->children()) {
        if (obj->inherits("DBusMenu")) {
            return true;
        }
    }
    return false;
}

// DBusMenuExporter adds an instance of an internal class named "DBusMenu" to
// any QMenu it tracks. Check they go away when the exporter is deleted.
void DBusMenuExporterTest::testDBusMenuObjectIsDeletedWhenExporterIsDeleted()
{
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    QAction *a1 = inputMenu.addAction("a1");
    QVERIFY2(hasInternalDBusMenuObject(&inputMenu), "Test setup failed");
    delete exporter;
    QVERIFY(!hasInternalDBusMenuObject(&inputMenu));
}

void DBusMenuExporterTest::testSeparatorCollapsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("one-separator")         << "a-b"           << "a-b";
    QTest::newRow("two-separators")        << "a-b-c"         << "a-b-c";
    QTest::newRow("middle-separators")     << "a--b"          << "a-b";
    QTest::newRow("separators-at-begin")   << "--a-b"         << "a-b";
    QTest::newRow("separators-at-end")     << "a-b--"         << "a-b";
    QTest::newRow("separators-everywhere") << "--a---bc--d--" << "a-bc-d";
    QTest::newRow("empty-menu")            << ""              << "";
    QTest::newRow("separators-only")       << "---"           << "";
}

void DBusMenuExporterTest::testSeparatorCollapsing()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    // Create menu from menu string
    QMenu inputMenu;

    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);

    if (input.isEmpty()) {
        // Pretend there was an action so that doEmitLayoutUpdated() is called
        // even if the new menu is empty. If we don't do this we don't test
        // DBusMenuExporterPrivate::collapseSeparators() for empty menus.
        delete inputMenu.addAction("dummy");
    }

    Q_FOREACH(QChar ch, input) {
        if (ch == '-') {
            inputMenu.addSeparator();
        } else {
            inputMenu.addAction(ch);
        }
    }

    QTest::qWait(500);

    // Check out exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    // Get exported menu info
    QStringList propertyNames = QStringList();
    DBusMenuLayoutItemList list = getChildren(&iface, /*parentId=*/0, propertyNames);

    // Recreate a menu string from the item list
    QString output;
    Q_FOREACH(const DBusMenuLayoutItem& item, list) {
        QVariantMap properties = item.properties;
        if (properties.contains("visible") && !properties.value("visible").toBool()) {
            continue;
        }
        QString type = properties.value("type").toString();
        if (type == "separator") {
            output += '-';
        } else {
            output += properties.value("label").toString();
        }
    }

    // Check it matches
    QCOMPARE(output, expected);
}

static void checkPropertiesChangedArgs(const QVariantList& args, const QString& name, const QVariant& value)
{
    QCOMPARE(args[0].toString(), QString("com.canonical.dbusmenu"));
    QVariantMap map;
    map.insert(name, value);
    QCOMPARE(args[1].toMap(), map);
    QCOMPARE(args[2].toStringList(), QStringList());
}

void DBusMenuExporterTest::testSetStatus()
{
    QMenu inputMenu;
    QVERIFY(QDBusConnection::sessionBus().registerService(TEST_SERVICE));
    DBusMenuExporter *exporter = new DBusMenuExporter(TEST_OBJECT_PATH, &inputMenu);
    ManualSignalSpy spy;
    QDBusConnection::sessionBus().connect(TEST_SERVICE, TEST_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", "sa{sv}as", &spy, SLOT(receiveCall(QString, QVariantMap, QStringList)));

    QTest::qWait(500);

    // Check our exporter is on DBus
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    QVERIFY2(iface.isValid(), qPrintable(iface.lastError().message()));

    QCOMPARE(exporter->status(), QString("normal"));

    // Change status, a DBus signal should be emitted
    exporter->setStatus("notice");
    QCOMPARE(exporter->status(), QString("notice"));
    QTest::qWait(500);
    QCOMPARE(spy.count(), 1);
    checkPropertiesChangedArgs(spy.takeFirst(), "Status", "notice");

    // Same status => no signal
    exporter->setStatus("notice");
    QTest::qWait(500);
    QCOMPARE(spy.count(), 0);

    // Change status, a DBus signal should be emitted
    exporter->setStatus("normal");
    QTest::qWait(500);
    QCOMPARE(spy.count(), 1);
    checkPropertiesChangedArgs(spy.takeFirst(), "Status", "normal");
}

void DBusMenuExporterTest::testGetIconDataProperty()
{
    // Create an icon
    QImage img(16, 16, QImage::Format_ARGB32);
    {
        QPainter painter(&img);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        QRect rect = img.rect();
        painter.fillRect(rect, Qt::transparent);
        rect.adjust(2, 2, -2, -2);
        painter.fillRect(rect, Qt::red);
        rect.adjust(2, 2, -2, -2);
        painter.fillRect(rect, Qt::green);
    }

    QIcon icon(QPixmap::fromImage(img));

    // Create a menu with the icon and export it
    QMenu inputMenu;
    QAction* a1 = inputMenu.addAction("a1");
    a1->setIcon(icon);
    DBusMenuExporter exporter(TEST_OBJECT_PATH, &inputMenu);

    // Get properties
    QDBusInterface iface(TEST_SERVICE, TEST_OBJECT_PATH);
    DBusMenuLayoutItemList layoutItemlist = getChildren(&iface, 0, QStringList());
    QCOMPARE(layoutItemlist.count(), 1);

    QList<int> ids = QList<int>() << layoutItemlist[0].id;

    QDBusReply<DBusMenuItemList> reply = iface.call("GetGroupProperties", QVariant::fromValue(ids), QStringList());

    DBusMenuItemList itemlist = reply.value();
    QCOMPARE(itemlist.count(), 1);

    // Check we have the right property
    DBusMenuItem item = itemlist.takeFirst();
    QVERIFY(!item.properties.contains("icon-name"));
    QVERIFY(item.properties.contains("icon-data"));

    // Check saved image is the same
    QByteArray data = item.properties.value("icon-data").toByteArray();
    QVERIFY(!data.isEmpty());
    QImage result;
    QVERIFY(result.loadFromData(data, "PNG"));
    QCOMPARE(result, img);
}

#include "dbusmenuexportertest.moc"
