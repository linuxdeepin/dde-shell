// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for Types (frame/dstypes), a minimal QObject wrapper used as a
// QML type registration anchor. Only a constructor — no branches.

#include <gtest/gtest.h>

#include <QObject>

#include "dstypes.h"

using namespace ds;

// Default-constructed Types is a valid QObject with no parent.
TEST(Types, DefaultConstruct)
{
    Types t;
    EXPECT_TRUE(t.metaObject()->inherits(&QObject::staticMetaObject));
    EXPECT_EQ(t.parent(), nullptr);
}

// Parent is wired through QObject's constructor.
TEST(Types, ConstructWithParent)
{
    QObject parent;
    auto *t = new Types(&parent);
    EXPECT_EQ(t->parent(), &parent);
    EXPECT_EQ(parent.children().count(), 1);
    delete t; // removing from parent manually; parent still owns it via deleteLater-safe pattern
}

// Destructor is safe — no crash, no leak when stack-allocated.
TEST(Types, DestructorSafety)
{
    EXPECT_NO_THROW({
        Types t;
    });
}
