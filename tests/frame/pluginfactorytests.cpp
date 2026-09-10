// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for DAppletFactory (frame/pluginfactory), the plugin registration
// helper. registerInstance()/create() key off metaObject()->className() into a
// file-scope static QMap, so each test uses a uniquely-named Q_OBJECT factory
// subclass to avoid cross-test collision in that global map. The registered
// CreateAppletFunction returns nullptr (plus a side-effect flag) so no DApplet
// needs to be instantiated and applet.cpp is not pulled in.

#include <gtest/gtest.h>

#include <QObject>
#include <QString>

#include "pluginfactory.h"

using namespace ds;

// --- Unique Q_OBJECT factory subclasses (one per test that registers) ---------
class InvokeFactory : public DAppletFactory
{
    Q_OBJECT
public:
    explicit InvokeFactory(QObject *parent = nullptr) : DAppletFactory(parent) {}
};

class DuplicateFactory : public DAppletFactory
{
    Q_OBJECT
public:
    explicit DuplicateFactory(QObject *parent = nullptr) : DAppletFactory(parent) {}
};

// registerInstance() inserts the function under the factory's className, and a
// subsequent create() invokes it. Covers the insert branch of registerInstance
// and the "found -> invoke" branch of create.
TEST(DAppletFactory, RegisterAndCreateInvokesFunction)
{
    InvokeFactory factory;

    bool invoked = false;
    QObject *capturedParent = reinterpret_cast<QObject *>(0xDEADBEEF); // sentinel
    factory.registerInstance([&invoked, &capturedParent](QObject *parent) -> DApplet * {
        invoked = true;
        capturedParent = parent;
        return nullptr; // no real DApplet needed to exercise the dispatch
    });

    // create() should look up the registered function and call it.
    QObject *sentinelParent = reinterpret_cast<QObject *>(0xCAFEBABE);
    DApplet *result = factory.create(sentinelParent);

    EXPECT_EQ(result, nullptr);       // our function returns nullptr
    EXPECT_TRUE(invoked);             // the function was actually dispatched
    EXPECT_EQ(capturedParent, sentinelParent); // parent forwarded
}

// create() on a factory that never registered returns nullptr. Covers the
// "not found -> nullptr" branch of create. Uses the base DAppletFactory (whose
// className "ds::DAppletFactory" is never registered by any test here).
TEST(DAppletFactory, CreateUnregisteredReturnsNull)
{
    DAppletFactory factory;
    EXPECT_EQ(factory.create(), nullptr);
}

// A second registerInstance() with the same className is ignored (the
// "already registered" branch); create() keeps using the first function.
TEST(DAppletFactory, DuplicateRegistrationIsIgnored)
{
    DuplicateFactory factory;

    bool firstCalled = false;
    bool secondCalled = false;

    factory.registerInstance([&firstCalled](QObject *) -> DApplet * {
        firstCalled = true;
        return nullptr;
    });
    // Second registration under the same className must be a no-op.
    factory.registerInstance([&secondCalled](QObject *) -> DApplet * {
        secondCalled = true;
        return nullptr;
    });

    EXPECT_EQ(factory.create(), nullptr);
    EXPECT_TRUE(firstCalled);  // the first function is the one dispatched
    EXPECT_FALSE(secondCalled); // the duplicate was ignored
}

// registerApplet<T> is a stateless template helper (new T(parent)); it is not
// exercised with a real DApplet here because that would pull applet.cpp into
// the build. The three tests above cover all branches of registerInstance()
// (insert / duplicate-ignore) and create() (found-invoke / not-found-null).

#include "pluginfactorytests.moc"
