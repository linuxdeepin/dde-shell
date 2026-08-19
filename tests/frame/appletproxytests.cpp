// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for DAppletProxy and DAppletMetaProxy (frame/appletproxy).
//
// DAppletProxy's constructor is protected, so it cannot be directly
// instantiated. DAppletMetaProxy (declared in the private header
// private/appletproxy_p.h) has a public constructor and is the concrete class
// we test. Its metaObject()/qt_metacast()/qt_metacall() overrides have
// interesting branching: when the wrapped `meta` QObject is null vs non-null.

#include <gtest/gtest.h>

#include <QObject>
#include <QMetaObject>
#include <QMetaMethod>
#include <QString>

// Private header that declares DAppletMetaProxy.
#include "private/appletproxy_p.h"

using namespace ds;

// A minimal Q_OBJECT subclass so its staticMetaObject differs from
// QObject's, letting us distinguish the meta-null vs meta-non-null branches.
class TestMetaObject : public QObject
{
    Q_OBJECT
public:
    explicit TestMetaObject(QObject *parent = nullptr) : QObject(parent) {}
};

// --- DAppletMetaProxy::metaObject() ---

// With a non-null meta, metaObject() returns the meta's metaObject.
TEST(DAppletMetaProxy, MetaObjectWithMeta)
{
    TestMetaObject meta;
    DAppletMetaProxy proxy(&meta, nullptr);
    EXPECT_EQ(proxy.metaObject(), meta.metaObject());
    EXPECT_NE(proxy.metaObject(), &QObject::staticMetaObject);
}

// With a null meta, metaObject() returns the base (DAppletProxy/QObject) staticMetaObject.
TEST(DAppletMetaProxy, MetaObjectWithoutMeta)
{
    DAppletMetaProxy proxy(nullptr, nullptr);
    EXPECT_EQ(proxy.metaObject(), &QObject::staticMetaObject);
}

// --- DAppletMetaProxy::qt_metacast() ---

// With a non-null meta, qt_metacast returns the meta pointer regardless of clname.
TEST(DAppletMetaProxy, MetaCastWithMeta)
{
    TestMetaObject meta;
    DAppletMetaProxy proxy(&meta, nullptr);
    void *result = proxy.qt_metacast("QObject");
    EXPECT_EQ(result, &meta);
}

// Without meta and a null clname, qt_metacast returns nullptr.
TEST(DAppletMetaProxy, MetaCastWithoutMetaNullName)
{
    DAppletMetaProxy proxy(nullptr, nullptr);
    EXPECT_EQ(proxy.qt_metacast(nullptr), nullptr);
}

// Without meta and a valid clname, qt_metacast delegates to QObject's implementation.
TEST(DAppletMetaProxy, MetaCastWithoutMetaValidName)
{
    DAppletMetaProxy proxy(nullptr, nullptr);
    void *result = proxy.qt_metacast("QObject");
    // QObject::qt_metacast("QObject") returns `this` if the object is a QObject.
    EXPECT_EQ(result, &proxy);
}

// --- DAppletMetaProxy::qt_metacall() ---

// Without meta, qt_metacall delegates directly to the base implementation.
TEST(DAppletMetaProxy, MetaCallWithoutMeta)
{
    DAppletMetaProxy proxy(nullptr, nullptr);
    // id=-1 → invalid → base returns -1.
    int result = proxy.qt_metacall(QMetaObject::WriteProperty, -1, nullptr);
    EXPECT_EQ(result, -1);
}

// With meta, qt_metacall calls meta's qt_metacall; if it returns < 0, falls through.
TEST(DAppletMetaProxy, MetaCallWithMetaFallthrough)
{
    TestMetaObject meta;
    DAppletMetaProxy proxy(&meta, nullptr);
    // id=-1 → meta's qt_metacall returns -1 (< 0) → falls through to base → -1.
    int result = proxy.qt_metacall(QMetaObject::WriteProperty, -1, nullptr);
    EXPECT_EQ(result, -1);
}

// Destructor is safe.
TEST(DAppletMetaProxy, DestructorSafety)
{
    EXPECT_NO_THROW({
        DAppletMetaProxy proxy(nullptr, nullptr);
    });
    EXPECT_NO_THROW({
        TestMetaObject meta;
        DAppletMetaProxy proxy(&meta, nullptr);
    });
}

#include "appletproxytests.moc"
