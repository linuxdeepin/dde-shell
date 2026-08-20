// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amappitem.h"
#include "appitem.h"
#include "appitemmodel.h"
#include "applicationinterface.h"
#include "categoryutils.h"

#include <DUtil>

namespace apps
{
// AM static string
static const QString AM_DBUS_SERVICE = "org.desktopspec.ApplicationManager1";
static const QString AM_APPLICATION_INTERFACE = "org.desktopspec.ApplicationManager1.Application";
static const QString DBUS_PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
static const QString DESKTOP_ENTRY_ICON_KEY = "Desktop Entry";
static const QString DEFAULT_KEY = "default";
static QString locale = QLocale::system().name();

AMAppItem::AMAppItem(const QDBusObjectPath &path, QObject *parent)
    : Application(AM_DBUS_SERVICE, path.path(), QDBusConnection::sessionBus(), parent)
    , AppItem(DUtil::unescapeFromObjectPath(path.path().split('/').last()), AppItemModel::AppItemType)
{
    QDBusConnection::sessionBus().connect(AM_DBUS_SERVICE,
                                          path.path(),
                                          DBUS_PROPERTIES_INTERFACE,
                                          QStringLiteral("PropertiesChanged"),
                                          QStringLiteral("sa{sv}as"),
                                          this,
                                          SLOT(onPropertyChanged(const QDBusMessage &)));
}

AMAppItem::AMAppItem(const QDBusObjectPath &path, const ObjectInterfaceMap &source, QObject *parent)
    : AMAppItem(path, parent)
{
    const QVariantMap appInfo = source.value("org.desktopspec.ApplicationManager1.Application");
    if (appInfo.isEmpty())
        return;

    auto name = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"Name")), locale, DEFAULT_KEY);
    auto genericName = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"GenericName")), locale, DEFAULT_KEY);
    auto xDeepinVendor = appInfo.value(u8"X_Deepin_Vendor").toString();
    AppItem::setGenericName(genericName);
    AppItem::setVendor(xDeepinVendor);

    if (QStringLiteral("deepin") == xDeepinVendor && !genericName.isEmpty()) {
        AppItem::setAppName(genericName);
    } else {
        AppItem::setAppName(name);
    }

    auto iconName = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"Icons")), DESKTOP_ENTRY_ICON_KEY, "");
    AppItem::setAppIconName(iconName);

    auto noDisplay = appInfo.value(u8"NoDisplay").toBool();
    AppItem::setNoDisPlay(noDisplay);

    auto categories = appInfo.value(u8"Categories").toStringList();
    AppItem::setDDECategories(AppItemModel::DDECategories(CategoryUtils::parseBestMatchedCategory(categories)));
    AppItem::setCategories(categories);

    auto lastLaunchedTime = appInfo.value(u8"LastLaunchedTime").toULongLong();
    AppItem::setLastLaunchedTime(lastLaunchedTime);

    auto installedTime = appInfo.value(u8"InstalledTime").toULongLong();
    AppItem::setInstalledTime(installedTime);

    auto startUpWMClass = appInfo.value(u8"StartupWMClass").toString();
    AppItem::setStartupWMclass(startUpWMClass);

    auto autoStart = appInfo.value(u8"AutoStart").toBool();
    AppItem::setAutoStart(autoStart);

    auto isOnDesktop = appInfo.value(u8"isOnDesktop").toBool();
    AppItem::setOnDesktop(isOnDesktop);

    PropMap actionName;
    appInfo.value(u8"ActionName").value<QDBusArgument>() >> actionName;

    auto actions = appInfo.value(u8"Actions").toStringList();
    updateActions(actions, actionName);

    auto isLingLong = appInfo.value(u8"X_linglong").toBool();
    AppItem::setXLingLong(isLingLong);

    auto id = appInfo.value(u8"ID").toString();
    AppItem::setId(id);

    auto XCreatedBy = appInfo.value(u8"X_CreatedBy").toString();
    AppItem::setXCreatedBy(XCreatedBy);

    auto execs = qdbus_cast<QStringMap>(appInfo.value(u8"Execs"));
    AppItem::setExecs(execs);

    auto desktopSourcePath = appInfo.value(u8"DesktopSourcePath").toString();
    AppItem::setDesktopSourcePath(desktopSourcePath);
}

void AMAppItem::launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    Application::Launch(action, fields, options);
    AppItem::launch();
}

void AMAppItem::setAutoStart(bool autoStart)
{
    Application::setAutoStart(autoStart);
    AppItem::setAutoStart(autoStart);
}

void AMAppItem::setOnDesktop(bool on)
{
    if (on) {
        Application::SendToDesktop();
    } else {
        Application::RemoveFromDesktop();
    }
    AppItem::setOnDesktop(on);
}

QString AMAppItem::getLocaleOrDefaultValue(const QStringMap &value, const QString &localeCode, const QString &fallbackKey)
{
    if (value.contains(localeCode)) {
        return value.value(localeCode);
    } else {
        QString fallbackValue = value.value(fallbackKey);
        if (localeCode.contains('_')) {
            QString prefix = localeCode.split('_')[0];
            return value.value(prefix, fallbackValue);
        } else {
            return fallbackValue;
        }
    }
}

void AMAppItem::onPropertyChanged(const QDBusMessage &msg)
{
    const QList<QVariant> arguments = msg.arguments();
    if (arguments.count() != 3)
        return;

    if (arguments.at(0).toString() != AM_APPLICATION_INTERFACE)
        return;

    QVariantMap changedProperties = qdbus_cast<QVariantMap>(arguments.at(1));

    const auto value = [&changedProperties](QLatin1StringView name) {
        return changedProperties.value(name);
    };
    const auto contains = [&changedProperties](QLatin1StringView name) {
        return changedProperties.contains(name);
    };

    if (contains(QLatin1String("Name")) || contains(QLatin1String("GenericName"))
        || contains(QLatin1String("X_Deepin_Vendor"))) {
        const QString name = getLocaleOrDefaultValue(
                contains(QLatin1String("Name"))
                        ? qdbus_cast<QStringMap>(value(QLatin1String("Name")))
                        : Application::name(),
                locale,
                DEFAULT_KEY);
        const QString genericName = getLocaleOrDefaultValue(
                contains(QLatin1String("GenericName"))
                        ? qdbus_cast<QStringMap>(value(QLatin1String("GenericName")))
                        : Application::genericName(),
                locale,
                DEFAULT_KEY);
        const QString vendor = contains(QLatin1String("X_Deepin_Vendor"))
                ? value(QLatin1String("X_Deepin_Vendor")).toString()
                : Application::x_Deepin_Vendor();
        AppItem::setGenericName(genericName);
        AppItem::setVendor(vendor);
        AppItem::setAppName(vendor == QLatin1String("deepin") && !genericName.isEmpty() ? genericName : name);
    }

    if (contains(QLatin1String("Icons"))) {
        const auto icons = qdbus_cast<QStringMap>(value(QLatin1String("Icons")));
        AppItem::setAppIconName(icons.value(DESKTOP_ENTRY_ICON_KEY));
    }
    if (contains(QLatin1String("NoDisplay")))
        AppItem::setNoDisPlay(value(QLatin1String("NoDisplay")).toBool());
    if (contains(QLatin1String("Categories"))) {
        const auto categories = value(QLatin1String("Categories")).toStringList();
        AppItem::setDDECategories(AppItemModel::DDECategories(CategoryUtils::parseBestMatchedCategory(categories)));
        AppItem::setCategories(categories);
    }
    if (contains(QLatin1String("LastLaunchedTime")))
        AppItem::setLastLaunchedTime(value(QLatin1String("LastLaunchedTime")).toULongLong());
    if (contains(QLatin1String("LaunchedTimes")))
        AppItem::setData(value(QLatin1String("LaunchedTimes")).toULongLong(), AppItemModel::LaunchedTimesRole);
    if (contains(QLatin1String("InstalledTime")))
        AppItem::setInstalledTime(value(QLatin1String("InstalledTime")).toULongLong());
    if (contains(QLatin1String("StartupWMClass")))
        AppItem::setStartupWMclass(value(QLatin1String("StartupWMClass")).toString());
    if (contains(QLatin1String("AutoStart")))
        AppItem::setAutoStart(value(QLatin1String("AutoStart")).toBool());
    if (contains(QLatin1String("isOnDesktop")))
        AppItem::setOnDesktop(value(QLatin1String("isOnDesktop")).toBool());
    if (contains(QLatin1String("X_linglong")))
        AppItem::setXLingLong(value(QLatin1String("X_linglong")).toBool());
    if (contains(QLatin1String("ID")))
        AppItem::setId(value(QLatin1String("ID")).toString());
    if (contains(QLatin1String("X_CreatedBy")))
        AppItem::setXCreatedBy(value(QLatin1String("X_CreatedBy")).toString());
    if (contains(QLatin1String("Execs")))
        AppItem::setExecs(qdbus_cast<QStringMap>(value(QLatin1String("Execs"))));
    if (contains(QLatin1String("DesktopSourcePath")))
        AppItem::setDesktopSourcePath(value(QLatin1String("DesktopSourcePath")).toString());

    if (contains(QLatin1String("Actions")) || contains(QLatin1String("ActionName"))) {
        const QStringList actions = contains(QLatin1String("Actions"))
                ? value(QLatin1String("Actions")).toStringList()
                : Application::actions();
        const PropMap actionNames = contains(QLatin1String("ActionName"))
                ? qdbus_cast<PropMap>(value(QLatin1String("ActionName")))
                : Application::actionName();
        updateActions(actions, actionNames);
    }
}

void AMAppItem::updateActions(const QStringList &actions, const PropMap &actionName)
{
    QJsonArray actionsArray;
    for (auto action : actions) {
        auto localeNames = actionName.value(action);
        QJsonObject actionObject;
        actionObject.insert(QStringLiteral("id"), action);
        actionObject.insert(QStringLiteral("name"), getLocaleOrDefaultValue(localeNames, locale, DEFAULT_KEY));
        actionsArray.append(actionObject);
    }
    AppItem::setActions(actions.isEmpty() ? QString() : QJsonDocument(actionsArray).toJson());
}
}
