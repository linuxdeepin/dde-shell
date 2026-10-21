// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amappitem.h"
#include <QDBusMetaType>
#include <QTest>

class AMAppItemTests : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QCOMPARE(QLocale::system().name(), QString("en_US"));
        qDBusRegisterMetaType<QStringMap>();
        qDBusRegisterMetaType<QStringListMap>();
        qDBusRegisterMetaType<PropMap>();
    }

    void localeFields_data()
    {
        QTest::addColumn<QStringMap>("genericNameMap");
        QTest::addColumn<QStringListMap>("keywords");
        QTest::addColumn<QString>("expectedGenericName");
        QTest::addColumn<QStringList>("expectedKeywords");
        QStringMap genericNameMap{{"default", "BaseGeneric"}, {"en_US", "ExactGeneric"},
                                {"en", "LanguageGeneric"}, {"zh_CN", "ChineseGeneric"}, {"de", "GermanGeneric"}};
        QStringListMap keywords{{"default", {"BaseKey", "SecondBaseKey"}},
                                {"en_US", {"ExactKey", "SecondExactKey"}},
                                {"en", {"LanguageKey"}}, {"zh_CN", {"ChineseKey"}}, {"de", {"GermanKey"}}};
        QTest::newRow("exact") << genericNameMap << keywords
            << QString("ExactGeneric")
            << QStringList{"ExactKey", "SecondExactKey"};
        genericNameMap.remove("en_US");
        keywords.remove("en_US");
        QTest::newRow("language") << genericNameMap << keywords
            << QString("LanguageGeneric")
            << QStringList{"LanguageKey"};
        // Upstream stops at an existing empty key; keep that policy for both types.
        genericNameMap.insert("en_US", "");
        keywords.insert("en_US", {});
        QTest::newRow("empty-exact") << genericNameMap << keywords
            << QString("") << QStringList{};
        genericNameMap.remove("en_US"); genericNameMap.remove("en");
        keywords.remove("en_US"); keywords.remove("en");
        QTest::newRow("default") << genericNameMap << keywords
            << QString("BaseGeneric") << QStringList{"BaseKey", "SecondBaseKey"};
        genericNameMap.insert("en_US", "  basegeneric  ");
        keywords.insert("en_US", {"basekey", " ", "escaped;semicolon", "back\\slash", "space word"});
        QTest::newRow("decoded-values-preserved") << genericNameMap << keywords
            << QString("  basegeneric  ")
            << QStringList{"basekey", " ", "escaped;semicolon", "back\\slash", "space word"};
    }

    void localeFields()
    {
        QFETCH(QStringMap, genericNameMap);
        QFETCH(QStringListMap, keywords);
        QFETCH(QString, expectedGenericName);
        QFETCH(QStringList, expectedKeywords);
        QVariantMap properties{
            {"Name", QVariant::fromValue(QStringMap{{"default", "DefaultName"}, {"en_US", "ExactName"}, {"en", "LanguageName"}})},
            {"GenericName", QVariant::fromValue(genericNameMap)},
            {"Keywords", QVariant::fromValue(keywords)},
            {"X_Deepin_Vendor", ""}
        };
        const QDBusObjectPath path("/org/desktopspec/ApplicationManager1/LocaleFixture");
        const QString interface = "org.desktopspec.ApplicationManager1.Application";
        apps::AMAppItem item(path, ObjectInterfaceMap{{interface, properties}});
        QCOMPARE(item.AppItem::genericName(), expectedGenericName);
        QCOMPARE(item.AppItem::keywords(), expectedKeywords);
        QCOMPARE(item.appName(), QString("ExactName"));

        // Exercise the other production entry point, replacing existing role values.
        item.setGenericName("StaleGeneric");
        item.setKeywords({"StaleKeyword"});
        QDBusMessage message = QDBusMessage::createSignal(path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged");
        message.setArguments({interface, properties, QStringList{}});
        QVERIFY(QMetaObject::invokeMethod(&item, "onPropertyChanged", Qt::DirectConnection, Q_ARG(QDBusMessage, message)));
        QCOMPARE(item.AppItem::genericName(), expectedGenericName);
        QCOMPARE(item.AppItem::keywords(), expectedKeywords);
        QCOMPARE(item.appName(), QString("ExactName"));
    }
};

QTEST_MAIN(AMAppItemTests)
#include "amappitemtests.moc"
