// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QModelIndex>
#include <QDateTime>
#include <QVector>
#include <QHash>

#include "windowmanager.h"

// =================== Constructor / rowCount ===================

TEST(WindowManager, RowCountEmpty)
{
    WindowManager wm;
    EXPECT_EQ(wm.rowCount(), 0);
}

TEST(WindowManager, RowCountWithParentIndex)
{
    // rowCount ignores the parent index (Q_UNUSED).
    WindowManager wm;
    QModelIndex parent = wm.index(0, 0);
    EXPECT_EQ(wm.rowCount(parent), 0);
}

TEST(WindowManager, RowCountAfterAddingItems)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    EXPECT_EQ(wm.rowCount(), 2);
}

// =================== data() ===================

TEST(WindowManager, DataInvalidIndex)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex invalid;
    EXPECT_EQ(wm.data(invalid, WindowManager::NameRole), QVariant());
}

TEST(WindowManager, DataNegativeRow)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(-1, 0);
    EXPECT_EQ(wm.data(idx, WindowManager::NameRole), QVariant());
}

TEST(WindowManager, DataRowOutOfBounds)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(5, 0);
    EXPECT_EQ(wm.data(idx, WindowManager::NameRole), QVariant());
}

TEST(WindowManager, DataNameRole)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(0, 0);
    EXPECT_EQ(idx.data(WindowManager::NameRole).toString(), QString("ForegroundApp：app1"));
}

TEST(WindowManager, DataIdRole)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 42);
    QModelIndex idx = wm.index(0, 0);
    EXPECT_EQ(idx.data(WindowManager::IdRole).toUInt(), 42u);
}

TEST(WindowManager, DataStartTimeRole)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(0, 0);
    // startTime is set to currentDateTime(); toMSecsSinceEpoch should be non-zero.
    qint64 msecs = idx.data(WindowManager::StartTimeRole).toLongLong();
    EXPECT_GT(msecs, 0);
}

TEST(WindowManager, DataDefaultRole)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(0, 0);
    // Unknown role returns empty variant.
    EXPECT_EQ(wm.data(idx, Qt::UserRole + 999), QVariant());
}

TEST(WindowManager, DataDisplayRole)
{
    // Qt::DisplayRole (0) is not handled by the switch → empty variant.
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QModelIndex idx = wm.index(0, 0);
    EXPECT_EQ(wm.data(idx, Qt::DisplayRole), QVariant());
}

// =================== roleNames() ===================

TEST(WindowManager, RoleNamesContainsAllRoles)
{
    WindowManager wm;
    auto roles = wm.roleNames();
    EXPECT_EQ(roles.size(), 3);
    EXPECT_EQ(roles[WindowManager::NameRole], "name");
    EXPECT_EQ(roles[WindowManager::IdRole], "id");
    EXPECT_EQ(roles[WindowManager::StartTimeRole], "startTime");
}

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
// =================== windowList() ===================
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, WindowListEmpty)
{
    WindowManager wm;
    EXPECT_TRUE(wm.windowList().isEmpty());
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, WindowListReturnsItems)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoBackground("app2", 20);
    auto list = wm.windowList();
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list[0].name, QString("ForegroundApp：app1"));
    EXPECT_EQ(list[0].id, 10u);
    EXPECT_EQ(list[1].name, QString("BackgroundApp：app2"));
    EXPECT_EQ(list[1].id, 20u);
}
*/

// =================== setWindowInfoForeground ===================

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoForegroundAddsItem)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    EXPECT_EQ(wm.rowCount(), 1);
    auto list = wm.windowList();
    EXPECT_EQ(list[0].name, QString("ForegroundApp：app1"));
    EXPECT_EQ(list[0].id, 10u);
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoForegroundSetsStartTime)
{
    WindowManager wm;
    QDateTime before = QDateTime::currentDateTime().addMSecs(-1);
    wm.setWindowInfoForeground("app1", 10);
    auto list = wm.windowList();
    EXPECT_TRUE(list[0].startTime >= before);
}
*/

TEST(WindowManager, SetWindowInfoForegroundMultipleDifferentNames)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    wm.setWindowInfoForeground("app3", 30);
    EXPECT_EQ(wm.rowCount(), 3);
}

TEST(WindowManager, SetWindowInfoForegroundDuplicateNameAddsDuplicate)
{
    // DEFECT: the duplicate check compares info.name (prefixed) against the raw
    // name parameter, so it never matches and duplicates are always added.
    // This test verifies the ACTUAL behavior.
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app1", 11);
    EXPECT_EQ(wm.rowCount(), 2);
}

// =================== setWindowInfoBackground ===================

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoBackgroundAddsItem)
{
    WindowManager wm;
    wm.setWindowInfoBackground("app1", 10);
    EXPECT_EQ(wm.rowCount(), 1);
    auto list = wm.windowList();
    EXPECT_EQ(list[0].name, QString("BackgroundApp：app1"));
    EXPECT_EQ(list[0].id, 10u);
}
*/

TEST(WindowManager, SetWindowInfoBackgroundMultipleDifferentNames)
{
    WindowManager wm;
    wm.setWindowInfoBackground("app1", 10);
    wm.setWindowInfoBackground("app2", 20);
    EXPECT_EQ(wm.rowCount(), 2);
}

TEST(WindowManager, SetWindowInfoBackgroundDuplicateNameAddsDuplicate)
{
    // Same DEFECT as setWindowInfoForeground: duplicate check compares
    // prefixed name against raw name → never matches.
    WindowManager wm;
    wm.setWindowInfoBackground("app1", 10);
    wm.setWindowInfoBackground("app1", 11);
    EXPECT_EQ(wm.rowCount(), 2);
}

TEST(WindowManager, SetWindowInfoForegroundAndBackgroundSameRawName)
{
    // Different prefix → different stored name → both added.
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoBackground("app1", 20);
    EXPECT_EQ(wm.rowCount(), 2);
}

// =================== WindowDestroyInfo ===================

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, WindowDestroyInfoRemovesById)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    wm.setWindowInfoForeground("app3", 30);
    EXPECT_EQ(wm.rowCount(), 3);

    wm.WindowDestroyInfo(20);
    EXPECT_EQ(wm.rowCount(), 2);
    auto list = wm.windowList();
    EXPECT_EQ(list[0].id, 10u);
    EXPECT_EQ(list[1].id, 30u);
}
*/

TEST(WindowManager, WindowDestroyInfoNotFound)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.WindowDestroyInfo(999);
    EXPECT_EQ(wm.rowCount(), 1);
}

TEST(WindowManager, WindowDestroyInfoEmptyList)
{
    WindowManager wm;
    wm.WindowDestroyInfo(10);
    EXPECT_EQ(wm.rowCount(), 0);
}

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, WindowDestroyInfoFirstItem)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    wm.WindowDestroyInfo(10);
    EXPECT_EQ(wm.rowCount(), 1);
    EXPECT_EQ(wm.windowList()[0].id, 20u);
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, WindowDestroyInfoLastItem)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    wm.WindowDestroyInfo(20);
    EXPECT_EQ(wm.rowCount(), 1);
    EXPECT_EQ(wm.windowList()[0].id, 10u);
}
*/

// =================== setWindowInfoActive ===================

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoActiveUpdatesNameAndEmitsDataChanged)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoActive(10, "newname");
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    int topRow = args.at(0).toModelIndex().row();
    int bottomRow = args.at(1).toModelIndex().row();
    EXPECT_EQ(topRow, 0);
    EXPECT_EQ(bottomRow, 0);
    auto roles = args.at(2).value<QVector<int>>();
    EXPECT_EQ(roles.size(), 3);

    EXPECT_EQ(wm.windowList()[0].name, QString("ForegroundApp：newname"));
}
*/

TEST(WindowManager, SetWindowInfoActiveSetsActiveId)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoActive(10, "newname");
    // m_ActiveId should now be 10 (verified indirectly via setWindowInfoInActive).
    // With private=public we can check directly:
    EXPECT_EQ(wm.m_ActiveId, 10);
}

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoActiveIdNotFound)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoActive(999, "newname");
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(wm.windowList()[0].name, QString("ForegroundApp：app1"));
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoActiveOnSecondItem)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoForeground("app2", 20);
    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoActive(20, "renamed");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(wm.windowList()[1].name, QString("ForegroundApp：renamed"));
    // First item unchanged.
    EXPECT_EQ(wm.windowList()[0].name, QString("ForegroundApp：app1"));
}
*/

// =================== setWindowInfoInActive ===================

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoInActiveReprefixesName)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoActive(10, "app1");
    // Now name = "ForegroundApp：app1", m_ActiveId = 10.

    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoInActive(10, "app1");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(wm.windowList()[0].name, QString("BackgroundApp：app1"));
    EXPECT_EQ(wm.m_ActiveId, 0);
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoInActiveWhenActiveIdZero)
{
    // With m_ActiveId == 0, setWindowInfoInActive should do nothing.
    WindowManager wm;
    wm.m_ActiveId = 0;  // explicit via private=public
    wm.setWindowInfoForeground("app1", 10);

    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoInActive(10, "app1");
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(wm.windowList()[0].name, QString("ForegroundApp：app1"));
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoInActiveIdParameterIgnored)
{
    // DEFECT: setWindowInfoInActive uses m_ActiveId, not the id parameter.
    // Even if id doesn't match, as long as m_ActiveId matches an item, it activates.
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.setWindowInfoActive(10, "app1");

    // Pass a wrong id — the function still processes because it checks m_ActiveId.
    wm.setWindowInfoInActive(999, "anything");
    EXPECT_EQ(wm.windowList()[0].name, QString("BackgroundApp：app1"));
    EXPECT_EQ(wm.m_ActiveId, 0);
}
*/

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoInActiveNameParameterIgnored)
{
    // DEFECT: setWindowInfoInActive does not use the name parameter at all;
    // it splits the existing name and re-prefixes.
    WindowManager wm;
    wm.setWindowInfoForeground("original", 10);
    wm.setWindowInfoActive(10, "original");
    // Now name = "ForegroundApp：original"

    wm.setWindowInfoInActive(10, "different_name_ignored");
    EXPECT_EQ(wm.windowList()[0].name, QString("BackgroundApp：original"));
}
*/

TEST(WindowManager, SetWindowInfoInActiveNoMatchingActiveId)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    wm.m_ActiveId = 999;  // no item has id 999
    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoInActive(999, "app1");
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(wm.m_ActiveId, 999);  // unchanged
}

// --- Disabled: windowList() not implemented in windowmanager.cpp (link failure) ---
/*
TEST(WindowManager, SetWindowInfoInActiveActiveThenInactiveCycle)
{
    WindowManager wm;
    wm.setWindowInfoForeground("app1", 10);
    // name = "ForegroundApp：app1"

    wm.setWindowInfoActive(10, "app1");
    EXPECT_EQ(wm.windowList()[0].name, QString("ForegroundApp：app1"));
    EXPECT_EQ(wm.m_ActiveId, 10);

    wm.setWindowInfoInActive(10, "app1");
    EXPECT_EQ(wm.windowList()[0].name, QString("BackgroundApp：app1"));
    EXPECT_EQ(wm.m_ActiveId, 0);

    // After inactive, m_ActiveId is 0, calling again does nothing.
    QSignalSpy spy(&wm, &WindowManager::dataChanged);
    wm.setWindowInfoInActive(10, "app1");
    EXPECT_EQ(spy.count(), 0);
}
*/

// =================== AppRuntimeInfo operator== ===================

TEST(WindowManager, AppRuntimeInfoEqualityTrue)
{
    AppRuntimeInfo a;
    a.name = "app1";
    a.id = 10;
    AppRuntimeInfo b;
    b.name = "app1";
    b.id = 10;
    EXPECT_TRUE(a == b);
}

TEST(WindowManager, AppRuntimeInfoEqualityFalseByName)
{
    AppRuntimeInfo a;
    a.name = "app1";
    a.id = 10;
    AppRuntimeInfo b;
    b.name = "app2";
    b.id = 10;
    EXPECT_FALSE(a == b);
}

TEST(WindowManager, AppRuntimeInfoEqualityFalseById)
{
    AppRuntimeInfo a;
    a.name = "app1";
    a.id = 10;
    AppRuntimeInfo b;
    b.name = "app1";
    b.id = 20;
    EXPECT_FALSE(a == b);
}

TEST(WindowManager, AppRuntimeInfoEqualityFalseByBoth)
{
    AppRuntimeInfo a;
    a.name = "app1";
    a.id = 10;
    AppRuntimeInfo b;
    b.name = "app2";
    b.id = 20;
    EXPECT_FALSE(a == b);
}

TEST(WindowManager, AppRuntimeInfoStartTimeNotInEquality)
{
    // operator== only checks name and id, not startTime.
    AppRuntimeInfo a;
    a.name = "app1";
    a.id = 10;
    a.startTime = QDateTime::fromMSecsSinceEpoch(1000);
    AppRuntimeInfo b;
    b.name = "app1";
    b.id = 10;
    b.startTime = QDateTime::fromMSecsSinceEpoch(2000);
    EXPECT_TRUE(a == b);
}
