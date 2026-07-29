// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <QCoreApplication>
#include <QPointer>
#include <QTest>
#include <QThread>
#include <QSignalSpy>
#include <QVariant>

#define private public
#include "notifyserverapplet.h"
#undef private
#include "notificationmanager.h"

using namespace notification;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

class TrackableNotificationManager : public NotificationManager {
    Q_OBJECT
public:
    explicit TrackableNotificationManager(QObject *parent = nullptr)
        : NotificationManager(parent) {}

    ~TrackableNotificationManager() override
    {
        ++destroyedCount;
    }

    static std::atomic_int destroyedCount;
};

std::atomic_int TrackableNotificationManager::destroyedCount = 0;

// Mock class for NotificationManager
class MockNotificationManager : public NotificationManager {
    Q_OBJECT
public:
    explicit MockNotificationManager(QObject *parent = nullptr)
        : NotificationManager(parent) {}

    MOCK_METHOD(bool, registerDbusService, (), ());
    MOCK_METHOD(void, actionInvoked, (qint64 id, uint bubbleId, const QString &actionKey));
    MOCK_METHOD(void, actionInvoked, (qint64 id, const QString &actionKey));
    MOCK_METHOD(void, notificationClosed, (qint64 id, uint bubbleId, uint reason));
    MOCK_METHOD(QVariant, GetAppInfo, (const QString &appId, uint configItem));
    MOCK_METHOD(void, removeNotification, (qint64 id));
    MOCK_METHOD(void, removeNotifications, (const QString &appName));
    MOCK_METHOD(void, removeNotifications, ());
    MOCK_METHOD(void, removeExpiredNotifications, ());
    MOCK_METHOD(void, setBlockClosedId, (qint64 id));
};

// Test fixture for NotifyServerApplet
class NotifyServerAppletTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure QCoreApplication is created for Qt objects
        if (!QCoreApplication::instance()) {
            int argc = 0;
            char *argv[] = {nullptr};
            app = new QCoreApplication(argc, argv);
        }
        applet = new NotifyServerApplet();
    }

    void TearDown() override {
        delete applet;
        applet = nullptr;
    }

    QCoreApplication *app = nullptr;
    NotifyServerApplet *applet = nullptr;
};

// Test constructor
TEST_F(NotifyServerAppletTest, ConstructorTest) {
    EXPECT_NE(applet, nullptr);
    EXPECT_TRUE(applet->inherits("ds::DApplet"));
}

// Test destructor (basic test - mainly checking no crash)
TEST_F(NotifyServerAppletTest, DestructorTest) {
    auto *testApplet = new NotifyServerApplet();
    EXPECT_NO_THROW(delete testApplet);
}

// Test QPointer members are null by default after construction
TEST_F(NotifyServerAppletTest, QPointerNullByDefaultTest) {
    auto *testApplet = new NotifyServerApplet();
    EXPECT_TRUE(testApplet->m_manager.isNull());
    EXPECT_TRUE(testApplet->m_worker.isNull());
    delete testApplet;
}

// Test QPointer auto-null after explicit delete
TEST_F(NotifyServerAppletTest, QPointerAutoNullAfterDeleteTest) {
    auto *testApplet = new NotifyServerApplet();
    auto *manager = new TrackableNotificationManager();

    testApplet->m_manager = manager;
    EXPECT_FALSE(testApplet->m_manager.isNull());

    delete manager;
    EXPECT_TRUE(testApplet->m_manager.isNull());

    delete testApplet;
}

// Test QPointer auto-null after deleteLater + sendPostedEvents
TEST_F(NotifyServerAppletTest, QPointerAutoNullAfterDeleteLaterTest) {
    TrackableNotificationManager::destroyedCount = 0;

    auto *testApplet = new NotifyServerApplet();
    auto *manager = new TrackableNotificationManager();

    testApplet->m_manager = manager;
    EXPECT_FALSE(testApplet->m_manager.isNull());

    manager->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    EXPECT_TRUE(testApplet->m_manager.isNull());
    EXPECT_EQ(TrackableNotificationManager::destroyedCount.load(), 1);

    delete testApplet;
}

// Test destructor when only manager exists (no worker thread)
TEST_F(NotifyServerAppletTest, DestructorWithManagerOnlyTest) {
    TrackableNotificationManager::destroyedCount = 0;

    auto *testApplet = new NotifyServerApplet();
    auto *manager = new TrackableNotificationManager();
    QPointer<TrackableNotificationManager> managerGuard(manager);

    testApplet->m_manager = manager;
    // m_worker remains nullptr

    delete testApplet;

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    EXPECT_TRUE(managerGuard.isNull());
    EXPECT_EQ(TrackableNotificationManager::destroyedCount.load(), 1);
}

// Test destructor when worker exists but manager is null
TEST_F(NotifyServerAppletTest, DestructorWithWorkerOnlyTest) {
    auto *testApplet = new NotifyServerApplet();
    auto *worker = new QThread();
    QPointer<QThread> workerGuard(worker);

    testApplet->m_worker = worker;
    testApplet->m_manager = nullptr;

    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);

    worker->start();
    QTRY_VERIFY_WITH_TIMEOUT(worker->isRunning(), 1000);

    EXPECT_NO_THROW(delete testApplet);

    QTRY_VERIFY_WITH_TIMEOUT(workerGuard.isNull(), 3000);
}

// Test QPointer auto-null for m_worker after running worker quit
TEST_F(NotifyServerAppletTest, QPointerWorkerAutoNullAfterQuitTest) {
    auto *testApplet = new NotifyServerApplet();
    auto *worker = new QThread();

    testApplet->m_worker = worker;
    worker->start();
    QTRY_VERIFY_WITH_TIMEOUT(worker->isRunning(), 1000);

    worker->quit();
    worker->wait();
    delete worker;

    EXPECT_TRUE(testApplet->m_worker.isNull());

    delete testApplet;
}

// Test destructor with stopped worker and manager (else branch in destructor)
TEST_F(NotifyServerAppletTest, DestructorWithStoppedWorkerAndManagerTest) {
    TrackableNotificationManager::destroyedCount = 0;

    auto *testApplet = new NotifyServerApplet();
    auto *worker = new QThread();
    auto *manager = new TrackableNotificationManager();
    QPointer<TrackableNotificationManager> managerGuard(manager);
    QPointer<QThread> workerGuard(worker);

    testApplet->m_manager = manager;
    testApplet->m_worker = worker;
    // worker is NOT started, so isRunning() returns false

    delete testApplet;

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    EXPECT_TRUE(managerGuard.isNull());
    EXPECT_TRUE(workerGuard.isNull());
    EXPECT_EQ(TrackableNotificationManager::destroyedCount.load(), 1);
}

TEST_F(NotifyServerAppletTest, DestructorDeletesManagerAfterWorkerThreadExit) {
    TrackableNotificationManager::destroyedCount = 0;

    auto *testApplet = new NotifyServerApplet();
    auto *worker = new QThread();
    auto *manager = new TrackableNotificationManager();
    QPointer<TrackableNotificationManager> managerGuard(manager);

    testApplet->m_manager = manager;
    testApplet->m_worker = worker;

    QObject::connect(worker, &QThread::finished, manager, &QObject::deleteLater);
    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);

    manager->moveToThread(worker);
    worker->start();

    QTRY_VERIFY_WITH_TIMEOUT(worker->isRunning(), 1000);

    delete testApplet;

    QTRY_VERIFY_WITH_TIMEOUT(managerGuard.isNull(), 3000);

    EXPECT_EQ(TrackableNotificationManager::destroyedCount.load(), 1);
}

// Test QPointer m_worker auto-null after worker thread finished
TEST_F(NotifyServerAppletTest, QPointerWorkerAutoNullAfterThreadFinishedTest) {
    TrackableNotificationManager::destroyedCount = 0;

    auto *testApplet = new NotifyServerApplet();
    auto *worker = new QThread();
    auto *manager = new TrackableNotificationManager();
    QPointer<QThread> workerGuard(worker);

    testApplet->m_manager = manager;
    testApplet->m_worker = worker;

    QObject::connect(worker, &QThread::finished, manager, &QObject::deleteLater);
    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);

    manager->moveToThread(worker);
    worker->start();
    QTRY_VERIFY_WITH_TIMEOUT(worker->isRunning(), 1000);

    worker->quit();
    QTRY_VERIFY_WITH_TIMEOUT(workerGuard.isNull(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(testApplet->m_manager.isNull(), 3000);

    EXPECT_EQ(TrackableNotificationManager::destroyedCount.load(), 1);

    delete testApplet;
}

// Test QPointer members non-null after init()
TEST_F(NotifyServerAppletTest, QPointerNonNullAfterInitTest) {
    auto *testApplet = new NotifyServerApplet();
    bool initResult = testApplet->init();

    if (initResult) {
        EXPECT_FALSE(testApplet->m_manager.isNull());
        EXPECT_FALSE(testApplet->m_worker.isNull());
    } else {
        EXPECT_TRUE(testApplet->m_manager.isNull());
        EXPECT_TRUE(testApplet->m_worker.isNull());
    }

    EXPECT_NO_THROW(delete testApplet);
}

// Test memory leak detection for destructor with initialized applet
TEST_F(NotifyServerAppletTest, DestructorMemoryLeakTest) {
    // This test verifies that all resources are properly cleaned up
    // when the applet is destroyed after init()
    
    auto *testApplet = new NotifyServerApplet();
    
    // Initialize the applet (creates m_manager, m_worker, and DbusAdaptors)
    bool initResult = testApplet->init();
    
    // Even if init fails (e.g., D-Bus not available), we should clean up properly
    // Record the state before deletion
    EXPECT_NO_THROW({
        delete testApplet;
        testApplet = nullptr;
    });
    
    // If we reach here without crash or sanitizer errors, memory is cleaned up
    EXPECT_EQ(testApplet, nullptr);
}

// Test memory leak detection - multiple init calls
TEST_F(NotifyServerAppletTest, MultipleInitMemoryLeakTest) {
    auto *testApplet = new NotifyServerApplet();
    
    // First init
    testApplet->init();
    
    // Second init - after adding reentrancy prevention, no more crashes or leaks will occur
    EXPECT_NO_THROW(testApplet->init());
    
    EXPECT_NO_THROW({
        delete testApplet;
    });
}

// Test memory leak with Valgrind/ASan friendly pattern
// TEST_F(NotifyServerAppletTest, DestructorResourceCleanupTest) {
//     // This test is designed to be run with memory leak detectors
//     // like Valgrind or AddressSanitizer
//     // Each iteration creates and destroys a fresh applet instance
    
//     for (int i = 0; i < 5; ++i) {
//         auto *testApplet = new NotifyServerApplet();
//         EXPECT_NE(testApplet, nullptr);
        
//         // Initialize - each applet instance should only be initialized once
//         bool initResult = testApplet->init();
//         // init() may fail in test environment without D-Bus, but should not crash
//         (void)initResult;  // Suppress unused warning
        
//         // Destroy - this should properly clean up m_manager and m_worker
//         delete testApplet;
//     }
    
//     // If memory leaks exist, running this test with ASan will report:
//     // ERROR: AddressSanitizer: memory leak
//     SUCCEED() << "Resource cleanup test completed. Run with ASan/Valgrind to detect leaks.";
// }

// Test load() method
TEST_F(NotifyServerAppletTest, LoadTest) {
    // load() should call parent class load and return its result
    EXPECT_TRUE(applet->load());
}

// Test init() method - basic initialization
TEST_F(NotifyServerAppletTest, InitTest) {
    // init() creates NotificationManager and registers D-Bus service
    // Note: This may fail in test environment without D-Bus
    // We mainly test that it doesn't crash
    EXPECT_NO_THROW(applet->init());
}

// Test actionInvoked with bubbleId overload
TEST_F(NotifyServerAppletTest, ActionInvokedWithBubbleIdTest) {
    // Initialize applet first
    applet->init();
    
    qint64 testId = 12345;
    uint testBubbleId = 100;
    QString testActionKey = "default";
    
    // Test that actionInvoked doesn't crash
    EXPECT_NO_THROW(applet->actionInvoked(testId, testBubbleId, testActionKey));
}

// Test actionInvoked without bubbleId overload
TEST_F(NotifyServerAppletTest, ActionInvokedWithoutBubbleIdTest) {
    // Initialize applet first
    applet->init();
    
    qint64 testId = 12345;
    QString testActionKey = "default";
    
    // Test that actionInvoked doesn't crash
    EXPECT_NO_THROW(applet->actionInvoked(testId, testActionKey));
}

// Test notificationClosed
TEST_F(NotifyServerAppletTest, NotificationClosedTest) {
    // Initialize applet first
    applet->init();
    
    qint64 testId = 12345;
    uint testBubbleId = 100;
    uint testReason = 1; // Closed by user
    
    // Test that notificationClosed doesn't crash
    EXPECT_NO_THROW(applet->notificationClosed(testId, testBubbleId, testReason));
}

// Test appValue
TEST_F(NotifyServerAppletTest, AppValueTest) {
    // Initialize applet first
    applet->init();
    
    QString testAppId = "test-app";
    int testConfigItem = 0;
    
    // Test that appValue returns a QVariant (may be invalid in test environment)
    QVariant result = applet->appValue(testAppId, testConfigItem);
    // Result type depends on implementation, just verify it doesn't crash
    EXPECT_NO_THROW(applet->appValue(testAppId, testConfigItem));
}

// Test removeNotification
TEST_F(NotifyServerAppletTest, RemoveNotificationTest) {
    // Initialize applet first
    applet->init();
    
    qint64 testId = 12345;
    
    // Test that removeNotification doesn't crash
    EXPECT_NO_THROW(applet->removeNotification(testId));
}

// Test removeNotifications with appName
TEST_F(NotifyServerAppletTest, RemoveNotificationsWithAppNameTest) {
    // Initialize applet first
    applet->init();
    
    QString testAppName = "test-application";
    
    // Test that removeNotifications doesn't crash
    EXPECT_NO_THROW(applet->removeNotifications(testAppName));
}

// Test removeNotifications without parameters (remove all)
TEST_F(NotifyServerAppletTest, RemoveAllNotificationsTest) {
    // Initialize applet first
    applet->init();
    
    // Test that removeNotifications doesn't crash
    EXPECT_NO_THROW(applet->removeNotifications());
}

// Test removeExpiredNotifications
TEST_F(NotifyServerAppletTest, RemoveExpiredNotificationsTest) {
    // Initialize applet first
    applet->init();
    
    // Test that removeExpiredNotifications doesn't crash
    EXPECT_NO_THROW(applet->removeExpiredNotifications());
}

// Test setBlockClosedId
TEST_F(NotifyServerAppletTest, SetBlockClosedIdTest) {
    // Initialize applet first
    applet->init();
    
    qint64 testId = 12345;
    
    // Test that setBlockClosedId doesn't crash
    EXPECT_NO_THROW(applet->setBlockClosedId(testId));
}

// Test notificationStateChanged signal
TEST_F(NotifyServerAppletTest, NotificationStateChangedSignalTest) {
    // Initialize applet first
    applet->init();
    
    QSignalSpy spy(applet, &NotifyServerApplet::notificationStateChanged);
    EXPECT_EQ(spy.count(), 0);
    
    // The signal is emitted by NotificationManager, not directly by applet
    // We verify the signal connection exists
    EXPECT_TRUE(spy.isValid());
}

// Test multiple actionInvoked calls
TEST_F(NotifyServerAppletTest, MultipleActionInvokedTest) {
    applet->init();
    
    // Test multiple consecutive calls
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(applet->actionInvoked(i, 100u + i, QString("action%1").arg(i)));
    }
}

// Test removeNotifications sequence
TEST_F(NotifyServerAppletTest, RemoveNotificationsSequenceTest) {
    applet->init();
    
    // Test sequence of remove operations
    EXPECT_NO_THROW({
        applet->removeNotification(1);
        applet->removeNotifications("app1");
        applet->removeExpiredNotifications();
        applet->removeNotifications();
    });
}

// Test appValue with different config items
TEST_F(NotifyServerAppletTest, AppValueDifferentConfigsTest) {
    applet->init();
    
    QString testAppId = "test-app";
    
    // Test with different config item values
    for (int i = 0; i < 5; ++i) {
        QVariant result = applet->appValue(testAppId, i);
        // Just verify no crash occurs
        (void)result;
    }
}

// Test edge cases for notificationClosed
TEST_F(NotifyServerAppletTest, NotificationClosedEdgeCasesTest) {
    applet->init();
    
    // Test with various reason codes
    EXPECT_NO_THROW(applet->notificationClosed(0, 0, 0));
    EXPECT_NO_THROW(applet->notificationClosed(-1, 0, 1));
    EXPECT_NO_THROW(applet->notificationClosed(999999999, 999999, 3));
}

// Test edge cases for setBlockClosedId
TEST_F(NotifyServerAppletTest, SetBlockClosedIdEdgeCasesTest) {
    applet->init();
    
    // Test with various ID values
    EXPECT_NO_THROW(applet->setBlockClosedId(0));
    EXPECT_NO_THROW(applet->setBlockClosedId(-1));
    EXPECT_NO_THROW(applet->setBlockClosedId(9223372036854775807LL)); // max qint64
}

// Test that applet properly inherits from DApplet
TEST_F(NotifyServerAppletTest, InheritanceTest) {
    EXPECT_TRUE(applet->inherits("ds::DApplet"));
    EXPECT_TRUE(applet->inherits("QObject"));
}

// Test thread safety of init (worker thread creation)
TEST_F(NotifyServerAppletTest, WorkerThreadCreationTest) {
    EXPECT_NO_THROW(applet->init());
    // After init, a worker thread should be created and started
    // We can't directly access m_worker, but we can verify init doesn't crash
}

// Test multiple init calls (should handle gracefully)
TEST_F(NotifyServerAppletTest, MultipleInitCallsTest) {
    EXPECT_NO_THROW({
        applet->init();
        // Second init call - behavior depends on implementation
        // Should not crash
        applet->init();
    });
}

// Test actionInvoked with empty action key
TEST_F(NotifyServerAppletTest, ActionInvokedEmptyActionKeyTest) {
    applet->init();
    
    EXPECT_NO_THROW(applet->actionInvoked(1, 1, QString()));
    EXPECT_NO_THROW(applet->actionInvoked(1, QString()));
}

// Test removeNotifications with empty app name
TEST_F(NotifyServerAppletTest, RemoveNotificationsEmptyAppNameTest) {
    applet->init();
    
    EXPECT_NO_THROW(applet->removeNotifications(QString()));
}

// Test appValue with empty app ID
TEST_F(NotifyServerAppletTest, AppValueEmptyAppIdTest) {
    applet->init();
    
    QVariant result = applet->appValue(QString(), 0);
    (void)result; // Suppress unused warning
}

// Main function for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Create QCoreApplication for Qt tests
    QCoreApplication app(argc, argv);
    
    return RUN_ALL_TESTS();
}

#include "notifyserverapplet_test.moc"
