// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for Utility (frame/dsutility), the base (non-X11) utility class.
//
// Utility's constructor is protected; the static instance() factory creates
// a base Utility (no BUILD_WITH_X11 in the test OBJECT lib). These tests
// exercise instance(), allChildrenWindows(), grabKeyboard(), grabMouse().
//
// A QGuiApplication is required (QGuiApplication::platformName(),
// qGuiApp->allWindows()). It is provided via a GTest global environment that
// sets QT_QPA_PLATFORM=offscreen and creates QGuiApplication before any test
// runs. This approach is compatible with gtest_discover_tests (each test is a
// separate process) and GTest::Main (no custom main needed).

#include <gtest/gtest.h>

#include <QGuiApplication>
#include <QWindow>
#include <QList>
#include <QString>

#include "dsutility.h"

using namespace ds;

// ---- Global test environment: creates QGuiApplication (offscreen) ----------
class QtGuiEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char arg0[] = "utilitytests";
        static char *argv[] = {arg0, nullptr};
        m_app = new QGuiApplication(argc, argv);
    }
    void TearDown() override
    {
        delete m_app;
        m_app = nullptr;
    }
private:
    QGuiApplication *m_app = nullptr;
};

// Static init: register the environment before main() runs.
::testing::Environment *const kQtEnv =
    ::testing::AddGlobalTestEnvironment(new QtGuiEnvironment);

// ---- Tests ------------------------------------------------------------------

// instance() returns a non-null Utility and is stable across calls.
TEST(Utility, InstanceReturnsNonNull)
{
    auto *u1 = Utility::instance();
    ASSERT_NE(u1, nullptr);
    auto *u2 = Utility::instance();
    EXPECT_EQ(u1, u2); // singleton — same pointer
}

// grabKeyboard / grabMouse are no-ops in the base class → return false.
TEST(Utility, GrabKeyboardMouseReturnFalse)
{
    auto *u = Utility::instance();
    EXPECT_FALSE(u->grabKeyboard(nullptr, true));
    EXPECT_FALSE(u->grabMouse(nullptr, true));
    EXPECT_FALSE(u->grabKeyboard(nullptr, false));
    EXPECT_FALSE(u->grabMouse(nullptr, false));
}

// allChildrenWindows with no windows returns an empty list.
TEST(Utility, AllChildrenWindowsEmpty)
{
    auto *u = Utility::instance();
    QWindow target;
    // No other windows have target as transient parent.
    auto result = u->allChildrenWindows(&target);
    EXPECT_TRUE(result.isEmpty());
}

// allChildrenWindows finds direct children (windows whose transientParent == target).
TEST(Utility, AllChildrenWindowsDirectChild)
{
    auto *u = Utility::instance();
    QWindow target;
    target.setObjectName(QStringLiteral("target"));
    QWindow child;
    child.setTransientParent(&target);
    child.setObjectName(QStringLiteral("child"));

    auto result = u->allChildrenWindows(&target);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.first(), &child);
}

// allChildrenWindows does NOT include the target itself.
TEST(Utility, AllChildrenWindowsExcludesTarget)
{
    auto *u = Utility::instance();
    QWindow target;
    auto result = u->allChildrenWindows(&target);
    // The target window is present in qGuiApp->allWindows() but its
    // transientParent is null, so the inner while loop ends without matching.
    EXPECT_FALSE(result.contains(&target));
}

// allChildrenWindows excludes unrelated windows.
TEST(Utility, AllChildrenWindowsExcludesUnrelated)
{
    auto *u = Utility::instance();
    QWindow target;
    QWindow unrelated;
    unrelated.setObjectName(QStringLiteral("unrelated"));
    // unrelated has no transient parent → not a child of target

    auto result = u->allChildrenWindows(&target);
    EXPECT_FALSE(result.contains(&unrelated));
}
