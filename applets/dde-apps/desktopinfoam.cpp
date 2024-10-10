// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktopinfoam.h"
#include "abstractdesktopinfo.h"

#include <QObject>
#include <DUtil>

namespace apps {
// AM static string
static const QString AM_DBUS_SERVICE = "org.desktopspec.ApplicationManager1";
static const QString DESKTOP_ENTRY_ICON_KEY = "Desktop Entry";
static const QString DEFAULT_KEY = "default";
static QString locale = QLocale::system().name();

DesktopInfoAM::DesktopInfoAM(const QString& path, QObject* parent)
    : AbstractDesktopInfo(parent)
    , m_path(path)
    , m_desktopInfo(new Application(AM_DBUS_SERVICE, m_path, QDBusConnection::sessionBus(), this))
{
    m_id = DUtil::unescapeFromObjectPath(m_path.split('/').last());
    QDBusConnection::sessionBus().connect(
        m_desktopInfo->service(),
        m_desktopInfo->path(),
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "sa{sv}as",
        this,
        SLOT(onPropertyChanged(const QDBusMessage &))
    );
}

DesktopInfoAM::DesktopInfoAM(const QString &amDbusPath, const ObjectInterfaceMap &source, QObject* parent)
    : DesktopInfoAM(amDbusPath, parent)
{
    const QVariantMap appInfo = source.value("org.desktopspec.ApplicationManager1.Application");
    if (appInfo.isEmpty())
        return;

    m_name = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"Name")), locale, DEFAULT_KEY);
    m_iconName = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"Icons")), DESKTOP_ENTRY_ICON_KEY, "");
    m_genericName = getLocaleOrDefaultValue(qdbus_cast<QStringMap>(appInfo.value(u8"GenericName")), locale, DEFAULT_KEY);
    m_nodisplay = appInfo.value(u8"NoDisplay").toBool();
    m_categies = appInfo.value(u8"Categories").toStringList();
    m_lastLaunchedTime = appInfo.value(u8"LastLaunchedTime").toUInt();
    m_installedTime = appInfo.value(u8"InstalledTime").toUInt();
    m_xDeepinVendor = appInfo.value(u8"X_Deepin_Vendor").toString();
    m_startUpWMClass = appInfo.value(u8"StartupWMClass").toString();
    m_autoStart = appInfo.value(u8"AutoStart").toBool();
    m_isOnDesktop = appInfo.value(u8"OnDesktop").toBool();
}

DesktopInfoAM::~DesktopInfoAM()
{
    qDebug() << "deleted " << desktopId();
}

void DesktopInfoAM::launch(const QString& action, const QStringList &fields, const QVariantMap &options)
{
    m_desktopInfo->Launch(action, fields, options);
}

QString DesktopInfoAM::desktopId() const
{
    return m_id;
}

QString DesktopInfoAM::name()
{
    return m_name;
}

QString DesktopInfoAM::iconName()
{
    return m_iconName;
}

QString DesktopInfoAM::genericName()
{
    return m_genericName;
}

QStringList DesktopInfoAM::categories()
{
    return m_categies;
}

bool DesktopInfoAM::nodisplay()
{
    return m_nodisplay;
}

QList<QPair<QString, QString>> DesktopInfoAM::actions()
{
    return m_actions;
}

qint64 DesktopInfoAM::lastLaunchedTime()
{
    return m_lastLaunchedTime;
}

qint64 DesktopInfoAM::installedTime()
{
    return m_installedTime;
}

qint64 DesktopInfoAM::launchedTimes()
{
    return m_launchedTimes;
}

QString DesktopInfoAM::deepinVendor()
{
    return m_xDeepinVendor;
}

QString DesktopInfoAM::startupWMClass()
{
    return m_startUpWMClass;
}

bool DesktopInfoAM::autoStart() const
{
    return m_autoStart;
}

void DesktopInfoAM::setAutoStart(bool autoStart)
{
    if (autoStart == m_autoStart) return;
    m_autoStart = autoStart;
    m_desktopInfo->setAutoStart(m_autoStart);
}

bool DesktopInfoAM::onDesktop() const
{
    return m_isOnDesktop;
}

void DesktopInfoAM::setOnDesktop(bool onDesktop)
{
    if (onDesktop == m_isOnDesktop) return;
    m_isOnDesktop = onDesktop;

    if (m_isOnDesktop) {
        m_desktopInfo->SendToDesktop();
    } else {
        m_desktopInfo->RemoveFromDesktop();
    }
}

QString DesktopInfoAM::getLocaleOrDefaultValue(const QStringMap &value, const QString &targetKey, const QString &fallbackKey)
{
    auto targetValue = value.value(targetKey);
    auto fallbackValue = value.value(fallbackKey);
    return !targetValue.isEmpty() ? targetValue : fallbackValue;
}

void DesktopInfoAM::onPropertyChanged(const QDBusMessage &msg)
{
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count())
        return;

    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName != QStringLiteral("org.desktopspec.ApplicationManager1.Application"))
        return;

    // AM send changed signal together
    auto name = getLocaleOrDefaultValue(m_desktopInfo->name(), locale, DEFAULT_KEY);
    if (name != m_name) {
        m_name = name;
        Q_EMIT nameChanged();
    }

    auto iconName = getLocaleOrDefaultValue(m_desktopInfo->icons(), DESKTOP_ENTRY_ICON_KEY, "");
    if (iconName != m_iconName) {
        m_iconName = iconName;
        Q_EMIT iconNameChanged();
    }

    auto genericName = getLocaleOrDefaultValue(m_desktopInfo->genericName(), locale, DEFAULT_KEY);
    if (genericName != m_genericName) {
        m_genericName = genericName;

        Q_EMIT nameChanged();
    }

    auto nodisplay = m_desktopInfo->noDisplay();
    if (nodisplay != m_nodisplay) {
        m_nodisplay = nodisplay;
        Q_EMIT nodisplayChanged();
    }

    auto categies = m_desktopInfo->categories();
    if (categies != m_categies) {
        m_categies = categies;
        Q_EMIT categoriesChanged();
    }

    auto lastLaunchedTime = m_desktopInfo->lastLaunchedTime();
    if (lastLaunchedTime != m_lastLaunchedTime) {
        m_lastLaunchedTime = lastLaunchedTime;
        Q_EMIT lastLaunchedTimeChanged();
    }

    auto installedTime = m_desktopInfo->installedTime();
    if (installedTime != m_installedTime) {
        m_installedTime = installedTime;
        Q_EMIT installedTimeChanged();
    }

    auto xDeepinVendor = m_desktopInfo->x_Deepin_Vendor();
    if (xDeepinVendor != m_xDeepinVendor) {
        m_xDeepinVendor = xDeepinVendor;
        Q_EMIT deepinVendorChanged();
    }

    auto startUpWMClass = m_desktopInfo->startupWMClass();
    if (startUpWMClass != m_startUpWMClass) {
        m_startUpWMClass = startUpWMClass;
        Q_EMIT startupWMClassChanged();
    }

    auto autoStart = m_desktopInfo->autoStart();
    if (autoStart != m_autoStart) {
        m_autoStart = autoStart;
        Q_EMIT autoStartChanged();
    }

    auto onDesktop = m_desktopInfo->isOnDesktop();
    if (onDesktop != m_isOnDesktop) {
        m_isOnDesktop = onDesktop;
        Q_EMIT onDesktopChanged();
    }
}
}