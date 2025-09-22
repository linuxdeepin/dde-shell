// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
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
static const QString DESKTOP_ENTRY_ICON_KEY = "Desktop Entry";
static const QString DEFAULT_KEY = "default";
static QString locale = QLocale::system().name();

AMAppItem::AMAppItem(const QDBusObjectPath &path, QObject *parent)
    : Application(AM_DBUS_SERVICE, path.path(), QDBusConnection::sessionBus(), parent)
    , AppItem(DUtil::unescapeFromObjectPath(path.path().split('/').last()), AppItemModel::AppItemType)
{
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

    auto lastLaunchedTime = appInfo.value(u8"LastLaunchedTime").toULongLong();
    AppItem::setLastLaunchedTime(lastLaunchedTime);

    auto installedTime = appInfo.value(u8"InstalledTime").toULongLong();
    AppItem::setInstalledTime(installedTime);

    auto startUpWMClass = appInfo.value(u8"StartupWMClass").toString();
    AppItem::setStartupWMclass(startUpWMClass);

    auto autoStart = appInfo.value(u8"AutoStart").toBool();
    AppItem::setAutoStart(autoStart);

    auto isOnDesktop = appInfo.value(u8"OnDesktop").toBool();
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
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count())
        return;

    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName != QStringLiteral("org.desktopspec.ApplicationManager1.Application"))
        return;

    // AM send changed signal together
    auto name = getLocaleOrDefaultValue(Application::name(), locale, DEFAULT_KEY);
    auto genericName = getLocaleOrDefaultValue(Application::genericName(), locale, DEFAULT_KEY);
    auto xDeepinVendor = Application::x_Deepin_Vendor();
    if (QStringLiteral("deepin") == xDeepinVendor && !genericName.isEmpty()) {
        AppItem::setAppName(genericName);
    } else {
        AppItem::setAppName(name);
    }

    auto iconName = Application::icons().value(DESKTOP_ENTRY_ICON_KEY);
    AppItem::setAppIconName(iconName);

    AppItem::setNoDisPlay(Application::noDisplay());
    AppItem::setDDECategories(AppItemModel::DDECategories(CategoryUtils::parseBestMatchedCategory(Application::categories())));
    AppItem::setLastLaunchedTime(Application::lastLaunchedTime());
    AppItem::setInstalledTime(Application::installedTime());
    AppItem::setStartupWMclass(Application::startupWMClass());
    AppItem::setAutoStart(Application::autoStart());
    AppItem::setOnDesktop(Application::isOnDesktop());
    AppItem::setXLingLong(Application::x_linglong());
    AppItem::setId(Application::iD());
    AppItem::setXCreatedBy(Application::x_CreatedBy());
    AppItem::setExecs(Application::execs());

    auto actions = Application::actions();
    auto actionName = Application::actionName();
    updateActions(actions, actionName);
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
    if (actions.size() > 0) {
        AppItem::setActions(QJsonDocument(actionsArray).toJson());
    }
}
}
