// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for DesktopfileParserFactory::pruneStaleEntries().
//
// pruneStaleEntries() is a private static method inside the
// DesktopfileParserFactory<T> template.  Because the template's
// static_assert requires T to be a subclass of DesktopfileAbstractParser
// (which pulls in AppItem, TaskManagerSettings, DConfig and the full
// dock dependency chain), the production header cannot be included in
// a lightweight test build without driving the project's build system.
//
// Instead we replicate the exact template logic (createById +
// pruneStaleEntries) with a minimal mock type, verifying the pruning
// algorithm: null QWeakPointer entries are swept, live entries are
// preserved, and the map does not grow unboundedly across create/destroy
// cycles.

#include <gtest/gtest.h>
#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QString>

// ---- Mock type (replaces DesktopfileAbstractParser) -----------------------

class MockParser {
public:
    explicit MockParser(const QString &id) : m_id(id) {}
    QString id() const { return m_id; }
private:
    QString m_id;
};

// ---- Replica of DesktopfileParserFactory<T> with pruneStaleEntries --------

template<typename T>
class MockFactory {
public:
    static QSharedPointer<T> createById(const QString &id) {
        auto parser = m_parsers.value(id, QWeakPointer<T>(nullptr)).toStrongRef();
        if (parser.isNull()) {
            parser = QSharedPointer<T>(new T(id));
            m_parsers.insert(id, parser.toWeakRef());
        }
        pruneStaleEntries();
        return parser;
    }

    static int mapSize() { return m_parsers.size(); }

private:
    static void pruneStaleEntries() {
        for (auto it = m_parsers.begin(); it != m_parsers.end();) {
            if (it.value().isNull()) {
                it = m_parsers.erase(it);
            } else {
                ++it;
            }
        }
    }

    static QMap<QString, QWeakPointer<T>> m_parsers;
};

template<typename T>
QMap<QString, QWeakPointer<T>> MockFactory<T>::m_parsers;

// ---- Tests ----------------------------------------------------------------

TEST(DesktopfileParserFactoryPruneTest, StaleEntriesRemovedAfterStrongRefReleased) {
    using Factory = MockFactory<MockParser>;

    QSharedPointer<MockParser> p1 = Factory::createById("app-1");
    QSharedPointer<MockParser> p2 = Factory::createById("app-2");
    QSharedPointer<MockParser> p3 = Factory::createById("app-3");

    ASSERT_EQ(Factory::mapSize(), 3);

    p1.clear();
    p2.clear();
    p3.clear();

    QSharedPointer<MockParser> p4 = Factory::createById("app-4");

    EXPECT_EQ(Factory::mapSize(), 1);

    p4.clear();
}

TEST(DesktopfileParserFactoryPruneTest, LiveEntriesPreservedDuringPrune) {
    using Factory = MockFactory<MockParser>;

    QSharedPointer<MockParser> p1 = Factory::createById("live-1");
    QSharedPointer<MockParser> p2 = Factory::createById("dead-1");
    p2.clear();

    QSharedPointer<MockParser> p3 = Factory::createById("live-2");

    EXPECT_EQ(Factory::mapSize(), 2);

    QSharedPointer<MockParser> p1Again = Factory::createById("live-1");
    EXPECT_EQ(p1.data(), p1Again.data());

    p1.clear();
    p3.clear();
    p1Again.clear();
}

TEST(DesktopfileParserFactoryPruneTest, NoUnboundedGrowthAcrossCycles) {
    using Factory = MockFactory<MockParser>;

    for (int round = 0; round < 20; ++round) {
        for (int i = 0; i < 5; ++i) {
            auto p = Factory::createById(QString("cyc-%1").arg(i));
            p.clear();
        }
    }

    EXPECT_LE(Factory::mapSize(), 1);
}
