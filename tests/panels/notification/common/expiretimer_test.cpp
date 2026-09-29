// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QDateTime>

#include "expiretimer.h"
#include "notifyentity.h"

using namespace notification;

// Helper to create a NotifyEntity with the given id, cTime, timeout and urgency.
static NotifyEntity makeEntity(qint64 id, qint64 cTime, int expireTimeout, int urgency = NotifyEntity::Normal)
{
    QVariantMap hints;
    if (urgency != NotifyEntity::Normal)
        hints.insert(QStringLiteral("urgency"), urgency);

    NotifyEntity entity(QStringLiteral("test-app"), 0, QString(), QString(),
                        QString(), QStringList(), hints, expireTimeout);
    entity.setId(id);
    entity.setCTime(cTime);
    return entity;
}

class ExpireTimerTest : public ::testing::Test {
protected:
    void SetUp() override {
        timer = ExpireTimer::instance();
    }

    void TearDown() override {
        timer->setBlockId(NotifyEntity::InvalidId);
    }

    ExpireTimer *timer = nullptr;
};

// Expiration timing: a notification pushed with a short timeout should emit
// the expired signal after that interval elapses.
TEST_F(ExpireTimerTest, ExpireAfterTimeout)
{
    NotifyEntity entity = makeEntity(1001, QDateTime::currentMSecsSinceEpoch(), 200);
    QSignalSpy spy(timer, &ExpireTimer::expired);

    timer->push(entity);

    spy.wait(2000);

    ASSERT_EQ(spy.count(), 1);
    const auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toLongLong(), 1001);
    EXPECT_EQ(args.at(1).toUInt(), entity.bubbleId());
}

// Idempotent push: pushing the same entity (same id + cTime) twice should not
// create a second countdown — only one expired signal is emitted.
TEST_F(ExpireTimerTest, IdempotentPushSameEntity)
{
    const qint64 cTime = QDateTime::currentMSecsSinceEpoch();
    NotifyEntity entity = makeEntity(2001, cTime, 300);
    QSignalSpy spy(timer, &ExpireTimer::expired);

    timer->push(entity);
    timer->push(entity);

    spy.wait(2000);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toLongLong(), 2001);
}

// Hover blocking: setBlockId prevents expiration while the id is blocked.
// After unblocking, the notification expires with a grace period.
TEST_F(ExpireTimerTest, BlockIdPreventsExpire)
{
    NotifyEntity entity = makeEntity(3001, QDateTime::currentMSecsSinceEpoch(), 200);
    QSignalSpy spy(timer, &ExpireTimer::expired);

    timer->push(entity);
    timer->setBlockId(3001);

    QTest::qWait(500);

    EXPECT_EQ(spy.count(), 0);

    timer->setBlockId(NotifyEntity::InvalidId);

    spy.wait(3000);

    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toLongLong(), 3001);
}

// Replacement cancellation: when a replacement notification (isReplace == true)
// with the same bubbleId is pushed, the old countdown is cancelled and the new
// one starts — the expired signal carries the new entity's id, not the old one.
TEST_F(ExpireTimerTest, ReplaceCancelsOldCountdown)
{
    const qint64 cTime = QDateTime::currentMSecsSinceEpoch();

    NotifyEntity original = makeEntity(4001, cTime, 5000);
    original.setBubbleId(42);
    timer->push(original);

    NotifyEntity replacement = makeEntity(4002, cTime, 200);
    replacement.setReplacesId(4001);
    replacement.setBubbleId(42);

    QSignalSpy spy(timer, &ExpireTimer::expired);
    timer->push(replacement);

    spy.wait(2000);

    ASSERT_GE(spy.count(), 1);
    bool foundReplacement = false;
    for (const auto &signal : spy) {
        if (signal.at(0).toLongLong() == 4002) {
            foundReplacement = true;
            break;
        }
    }
    EXPECT_TRUE(foundReplacement);

    for (const auto &signal : spy) {
        EXPECT_NE(signal.at(0).toLongLong(), 4001);
    }
}

// Never-expire: a Critical urgency notification should never emit expired.
TEST_F(ExpireTimerTest, CriticalUrgencyNeverExpires)
{
    NotifyEntity entity = makeEntity(5001, QDateTime::currentMSecsSinceEpoch(), 200, NotifyEntity::Critical);
    QSignalSpy spy(timer, &ExpireTimer::expired);

    timer->push(entity);

    QTest::qWait(1000);

    EXPECT_EQ(spy.count(), 0);
    timer->remove(5001);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QCoreApplication app(argc, argv);
    return RUN_ALL_TESTS();
}

#include "expiretimer_test.moc"
