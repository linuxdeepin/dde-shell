// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fashionleftpluginprovider.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QLocale>
#include <QNetworkInterface>
#include <QProcess>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>
#include <QSettings>
#include <QTimer>

#include <limits>
#include <utility>

namespace dock {

namespace {

constexpr auto NotificationService = "org.deepin.dde.Notification1";
constexpr auto NotificationPath = "/org/deepin/dde/Notification1";
constexpr auto NotificationInterface = "org.deepin.dde.Notification1";

constexpr auto MailService = "org.deepin.mail";
constexpr auto MailPath = "/org/deepin/mail";
constexpr auto MailInterface = "org.deepin.mail";

constexpr auto MprisPath = "/org/mpris/MediaPlayer2";
constexpr auto MprisRootInterface = "org.mpris.MediaPlayer2";
constexpr auto MprisPlayerInterface = "org.mpris.MediaPlayer2.Player";
constexpr auto DBusPropertiesInterface = "org.freedesktop.DBus.Properties";

constexpr auto SystemMonitorService = "org.deepin.SystemMonitorDaemon";
constexpr auto SystemMonitorPath = "/org/deepin/SystemMonitorDaemon";
constexpr auto SystemMonitorInterface = "org.deepin.SystemMonitorDaemon";

constexpr auto ControlCenterService = "org.deepin.dde.ControlCenter1";
constexpr auto ControlCenterPath = "/org/deepin/dde/ControlCenter1";
constexpr auto ControlCenterInterface = "org.deepin.dde.ControlCenter1";

constexpr auto StatusNotifierWatcherService = "org.kde.StatusNotifierWatcher";
constexpr auto StatusNotifierWatcherPath = "/StatusNotifierWatcher";
constexpr auto StatusNotifierWatcherInterface = "org.kde.StatusNotifierWatcher";
constexpr auto StatusNotifierItemInterface = "org.kde.StatusNotifierItem";

constexpr auto SystemMonitorServerService = "com.deepin.SystemMonitorServer";
constexpr auto SystemMonitorServerPath = "/com/deepin/SystemMonitorServer";
constexpr auto SystemMonitorServerInterface = "com.deepin.SystemMonitorServer";
constexpr auto SystemMonitorMainService = "com.deepin.SystemMonitorMain";
constexpr auto SystemMonitorMainPath = "/com/deepin/SystemMonitorMain";
constexpr auto SystemMonitorMainInterface = "com.deepin.SystemMonitorMain";

const QString WeatherAppIconPath = QStringLiteral("/usr/share/icons/hicolor/scalable/apps/org.deepin.weather.svg");
const QString WeatherDockPluginPath = QStringLiteral("/usr/lib/dde-dock/plugins/system-trays/libdeepin-weather-dock-plugin.so");
const QString MessageIconPath = QStringLiteral("/usr/share/icons/bloom/actions/24/mail-unread-new.svg");
const QString DefaultMailIconName = QStringLiteral("deepin-mail");
const QString DefaultMusicIconName = QStringLiteral("audio-x-generic");

using WeatherCodeToDescriptionFunction = QString (*)(int);
using WeatherCodeToIconNameFunction = QString (*)(int, bool);

struct MusicSnapshot
{
    QString service;
    QString desktopEntry;
    QString appName;
    QString title = QStringLiteral("未检测到音乐");
    QString subtitle = QStringLiteral("打开播放器开始播放");
    QUrl artSource;
    bool available = false;
    bool playing = false;
    bool canRaise = false;
    bool canGoPrevious = false;
    bool canGoNext = false;
    bool canTogglePlayback = false;
    int score = std::numeric_limits<int>::min();
};

struct MprisMetadataFallback
{
    QString title;
    QString artist;
    QString artUrl;
};

QString firstExistingPath(const QStringList &paths)
{
    for (const QString &path : paths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return {};
}

QStringList uniqueExistingDirectories(const QStringList &paths)
{
    QStringList directories;
    for (const QString &path : paths) {
        const QString cleanPath = QDir(path).absolutePath();
        if (directories.contains(cleanPath) || !QFileInfo(cleanPath).isDir()) {
            continue;
        }

        directories << cleanPath;
    }

    return directories;
}

bool querySystemMonitorUsage(const char *methodName, int *value)
{
    if (!value) {
        return false;
    }

    QDBusInterface systemMonitor(SystemMonitorService,
                                 SystemMonitorPath,
                                 SystemMonitorInterface,
                                 QDBusConnection::sessionBus());
    if (!systemMonitor.isValid()) {
        return false;
    }

    const QDBusReply<int> reply = systemMonitor.call(QString::fromLatin1(methodName));
    if (!reply.isValid()) {
        return false;
    }

    *value = qBound(0, reply.value(), 100);
    return true;
}

bool isUpAndRunningInterface(const QNetworkInterface &networkInterface)
{
    const auto flags = networkInterface.flags();
    return (flags & QNetworkInterface::IsUp)
        && (flags & QNetworkInterface::IsRunning)
        && !(flags & QNetworkInterface::IsLoopBack)
        && !networkInterface.name().trimmed().isEmpty();
}

bool isLikelyPhysicalTrafficInterface(const QNetworkInterface &networkInterface)
{
    if (!isUpAndRunningInterface(networkInterface)) {
        return false;
    }

    const QDir interfaceDirectory(QStringLiteral("/sys/class/net/%1").arg(networkInterface.name().trimmed()));
    if (!interfaceDirectory.exists()) {
        return false;
    }

    return QFileInfo(interfaceDirectory.filePath(QStringLiteral("device"))).exists()
        || QFileInfo(interfaceDirectory.filePath(QStringLiteral("wireless"))).exists();
}

QStringList defaultRouteInterfaceNames()
{
    QStringList routeInterfaceNames;
    QFile routeFile(QStringLiteral("/proc/net/route"));
    if (!routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return routeInterfaceNames;
    }

    while (!routeFile.atEnd()) {
        const QString line = QString::fromUtf8(routeFile.readLine()).simplified();
        if (line.startsWith(QStringLiteral("Iface"))) {
            continue;
        }

        const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 2) {
            continue;
        }

        const QString interfaceName = fields.at(0).trimmed();
        const QString destination = fields.at(1).trimmed();
        if (destination == QStringLiteral("00000000")
            && interfaceName != QStringLiteral("lo")
            && !routeInterfaceNames.contains(interfaceName)) {
            routeInterfaceNames << interfaceName;
        }
    }

    return routeInterfaceNames;
}

QVariantMap variantMapFromDBusArgument(const QDBusArgument &argument)
{
    QVariantMap result;
    if (argument.currentType() != QDBusArgument::MapType) {
        return result;
    }

    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QDBusVariant value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        result.insert(key, value.variant());
    }
    argument.endMap();
    return result;
}

QVariant unwrapDBusValue(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return qvariant_cast<QDBusVariant>(value).variant();
    }

    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument argument = qvariant_cast<QDBusArgument>(value);
        if (argument.currentType() == QDBusArgument::VariantType) {
            QDBusVariant unwrappedValue;
            argument >> unwrappedValue;
            return unwrapDBusValue(unwrappedValue.variant());
        }
    }

    return value;
}

QVariantMap dbusProperties(const QString &service, const QString &path, const QString &interfaceName)
{
    QDBusInterface propertiesInterface(service,
                                       path,
                                       DBusPropertiesInterface,
                                       QDBusConnection::sessionBus());
    const QDBusReply<QVariantMap> reply = propertiesInterface.call(QStringLiteral("GetAll"), interfaceName);
    return reply.isValid() ? reply.value() : QVariantMap();
}

QVariantMap dbusProperties(const QString &service, const QString &interfaceName)
{
    return dbusProperties(service, QLatin1String(MprisPath), interfaceName);
}

QString stringFromDBusValue(const QVariant &value)
{
    return unwrapDBusValue(value).toString().trimmed();
}

QStringList stringListFromDBusValue(const QVariant &value)
{
    const QVariant unwrappedValue = unwrapDBusValue(value);
    if (unwrappedValue.canConvert<QStringList>()) {
        return unwrappedValue.toStringList();
    }

    QStringList strings;
    const QVariantList values = unwrappedValue.toList();
    for (const QVariant &entry : values) {
        const QString text = stringFromDBusValue(entry);
        if (!text.isEmpty()) {
            strings << text;
        }
    }

    return strings;
}

bool boolFromDBusValue(const QVariant &value)
{
    return unwrapDBusValue(value).toBool();
}

QVariantMap mapFromDBusValue(const QVariant &value)
{
    const QVariant unwrappedValue = unwrapDBusValue(value);
    if (unwrappedValue.userType() == qMetaTypeId<QDBusArgument>()) {
        return variantMapFromDBusArgument(qvariant_cast<QDBusArgument>(unwrappedValue));
    }

    if (unwrappedValue.canConvert<QVariantMap>()) {
        return unwrappedValue.toMap();
    }

    return {};
}

bool callDBusMethod(const QString &service,
                    const QString &path,
                    const QString &interfaceName,
                    const QString &method,
                    const QVariantList &arguments = {})
{
    QDBusInterface interface(service, path, interfaceName, QDBusConnection::sessionBus());
    if (!interface.isValid()) {
        return false;
    }

    const QDBusMessage reply = interface.callWithArgumentList(QDBus::Block, method, arguments);
    return reply.type() != QDBusMessage::ErrorMessage;
}

bool splitStatusNotifierItemReference(const QString &itemReference, QString *service, QString *path)
{
    if (!service || !path) {
        return false;
    }

    const int pathSeparatorIndex = itemReference.indexOf(QLatin1Char('/'));
    if (pathSeparatorIndex <= 0) {
        return false;
    }

    *service = itemReference.left(pathSeparatorIndex).trimmed();
    *path = itemReference.mid(pathSeparatorIndex).trimmed();
    return !service->isEmpty() && !path->isEmpty();
}

bool activateStatusNotifierItem(const QString &expectedId, int x, int y)
{
    const QVariantMap watcherProperties = dbusProperties(QLatin1String(StatusNotifierWatcherService),
                                                         QLatin1String(StatusNotifierWatcherPath),
                                                         QLatin1String(StatusNotifierWatcherInterface));
    const QStringList registeredItems = stringListFromDBusValue(watcherProperties.value(QStringLiteral("RegisteredStatusNotifierItems")));
    for (const QString &itemReference : registeredItems) {
        QString service;
        QString path;
        if (!splitStatusNotifierItemReference(itemReference, &service, &path)) {
            continue;
        }

        const QVariantMap itemProperties = dbusProperties(service, path, QLatin1String(StatusNotifierItemInterface));
        if (stringFromDBusValue(itemProperties.value(QStringLiteral("Id"))) != expectedId) {
            continue;
        }

        return callDBusMethod(service,
                              path,
                              QLatin1String(StatusNotifierItemInterface),
                              QStringLiteral("Activate"),
                              {x, y});
    }

    return false;
}

bool runCommand(const QString &program, const QStringList &arguments = {}, int timeoutMs = 1500)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(500);
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QString processOutput(const QString &program, const QStringList &arguments = {}, int timeoutMs = 1500)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(500);
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QString desktopEntryText(const QString &desktopFilePath, const QString &key)
{
    if (desktopFilePath.isEmpty() || key.isEmpty()) {
        return {};
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));
    return settings.value(key).toString().trimmed();
}

QString localizedDesktopEntryText(const QString &desktopFilePath, const QString &key)
{
    if (desktopFilePath.isEmpty() || key.isEmpty()) {
        return {};
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));

    QStringList localizedKeys;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &uiLanguage : uiLanguages) {
        QString normalizedLanguage = uiLanguage.trimmed();
        if (normalizedLanguage.isEmpty()) {
            continue;
        }

        normalizedLanguage.replace(QLatin1Char('-'), QLatin1Char('_'));
        const QString fullKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage);
        if (!localizedKeys.contains(fullKey)) {
            localizedKeys << fullKey;
        }

        const int separatorIndex = normalizedLanguage.indexOf(QLatin1Char('_'));
        if (separatorIndex > 0) {
            const QString baseLanguageKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage.left(separatorIndex));
            if (!localizedKeys.contains(baseLanguageKey)) {
                localizedKeys << baseLanguageKey;
            }
        }
    }

    for (const QString &localizedKey : localizedKeys) {
        const QString localizedValue = settings.value(localizedKey).toString().trimmed();
        if (!localizedValue.isEmpty()) {
            return localizedValue;
        }
    }

    return settings.value(key).toString().trimmed();
}

QString desktopEntryExecutable(const QString &desktopFilePath)
{
    const QStringList commandLines = {
        desktopEntryText(desktopFilePath, QStringLiteral("TryExec")),
        desktopEntryText(desktopFilePath, QStringLiteral("Exec")),
    };

    for (const QString &commandLine : commandLines) {
        const QStringList commandParts = QProcess::splitCommand(commandLine);
        for (const QString &part : commandParts) {
            const QString token = part.trimmed();
            if (token.isEmpty() || token == QStringLiteral("env") || token.startsWith(QLatin1Char('%'))) {
                continue;
            }

            if (token.contains(QLatin1Char('='))
                && !token.startsWith(QLatin1Char('/'))
                && !token.contains(QDir::separator())) {
                continue;
            }

            return token;
        }
    }

    return {};
}

bool canManipulateWindows()
{
    static const bool available = !QStandardPaths::findExecutable(QStringLiteral("xdotool")).isEmpty();
    static const bool x11Session = qEnvironmentVariable("XDG_SESSION_TYPE").compare(QStringLiteral("x11"), Qt::CaseInsensitive) == 0
        || QGuiApplication::platformName().contains(QStringLiteral("xcb"), Qt::CaseInsensitive);
    return available && x11Session;
}

uint serviceProcessId(const QString &serviceName)
{
    if (serviceName.isEmpty()) {
        return 0;
    }

    QDBusConnectionInterface *connectionInterface = QDBusConnection::sessionBus().interface();
    if (!connectionInterface) {
        return 0;
    }

    const QDBusReply<uint> pidReply = connectionInterface->servicePid(serviceName);
    return pidReply.isValid() ? pidReply.value() : 0;
}

QStringList outputLines(const QString &output)
{
    QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString &line : lines) {
        line.remove(QLatin1Char('\r'));
        line = line.trimmed();
    }
    lines.removeAll(QString());
    return lines;
}

bool windowGeometry(const QString &windowId, QRect *geometry)
{
    if (!geometry || windowId.isEmpty() || !canManipulateWindows()) {
        return false;
    }

    const QString output = processOutput(QStringLiteral("xdotool"),
                                                                    {QStringLiteral("getwindowgeometry"),
                                                                     QStringLiteral("--shell"),
                                                                     windowId},
                                                                    1200);
    if (output.isEmpty()) {
        return false;
    }

    const QRegularExpression xPattern(QStringLiteral("(?:^|\\n)X=(-?\\d+)(?:\\n|$)"));
    const QRegularExpression yPattern(QStringLiteral("(?:^|\\n)Y=(-?\\d+)(?:\\n|$)"));
    const QRegularExpression widthPattern(QStringLiteral("(?:^|\\n)WIDTH=(\\d+)(?:\\n|$)"));
    const QRegularExpression heightPattern(QStringLiteral("(?:^|\\n)HEIGHT=(\\d+)(?:\\n|$)"));

    const auto xMatch = xPattern.match(output);
    const auto yMatch = yPattern.match(output);
    const auto widthMatch = widthPattern.match(output);
    const auto heightMatch = heightPattern.match(output);
    if (!xMatch.hasMatch() || !yMatch.hasMatch() || !widthMatch.hasMatch() || !heightMatch.hasMatch()) {
        return false;
    }

    *geometry = QRect(xMatch.captured(1).toInt(),
                      yMatch.captured(1).toInt(),
                      widthMatch.captured(1).toInt(),
                      heightMatch.captured(1).toInt());
    return geometry->isValid();
}

QString bestWindowIdForSearches(const QList<QStringList> &searches, bool onlyVisible = true)
{
    if (!canManipulateWindows()) {
        return {};
    }

    QString bestWindowId;
    int bestArea = -1;
    for (const QStringList &searchArguments : searches) {
        QStringList arguments = {QStringLiteral("search")};
        if (onlyVisible) {
            arguments << QStringLiteral("--onlyvisible");
        }
        arguments << searchArguments;
        const QStringList windowIds = outputLines(processOutput(QStringLiteral("xdotool"),
                                                                                           arguments,
                                                                                           1200));
        for (const QString &windowId : windowIds) {
            QRect geometry;
            if (!windowGeometry(windowId, &geometry)) {
                continue;
            }

            const int area = geometry.width() * geometry.height();
            if (area > bestArea) {
                bestArea = area;
                bestWindowId = windowId;
            }
        }
    }

    return bestWindowId;
}

bool activateWindow(const QString &windowId)
{
    return !windowId.isEmpty()
        && runCommand(QStringLiteral("xdotool"), {QStringLiteral("windowactivate"), QStringLiteral("--sync"), windowId}, 1200);
}

bool moveWindow(const QString &windowId, int x, int y)
{
    return !windowId.isEmpty()
        && runCommand(QStringLiteral("xdotool"),
                      {QStringLiteral("windowmove"), windowId, QString::number(x), QString::number(y)},
                      1200);
}

QString bestWindowIdForDesktopEntry(const QString &desktopFilePath,
                                    const QString &fallbackAppName,
                                    const QString &fallbackExecutablePath,
                                    bool onlyVisible = true)
{
    QList<QStringList> searches;
    if (!desktopFilePath.isEmpty()) {
        const QString startupWmClass = desktopEntryText(desktopFilePath, QStringLiteral("StartupWMClass"));
        if (!startupWmClass.isEmpty()) {
            searches << QStringList{QStringLiteral("--class"), startupWmClass};
        }

        const QString localizedName = localizedDesktopEntryText(desktopFilePath, QStringLiteral("Name"));
        if (!localizedName.isEmpty()) {
            searches << QStringList{QStringLiteral("--name"), localizedName};
            searches << QStringList{QStringLiteral("--class"), localizedName};
        }

        const QString genericName = localizedDesktopEntryText(desktopFilePath, QStringLiteral("GenericName"));
        if (!genericName.isEmpty()) {
            searches << QStringList{QStringLiteral("--name"), genericName};
        }

        const QString desktopExecutable = desktopEntryExecutable(desktopFilePath);
        const QString executableBaseName = QFileInfo(desktopExecutable).fileName();
        if (!executableBaseName.isEmpty()) {
            searches << QStringList{QStringLiteral("--class"), executableBaseName};
            searches << QStringList{QStringLiteral("--name"), executableBaseName};
        }
    }

    if (!fallbackAppName.isEmpty()) {
        searches << QStringList{QStringLiteral("--name"), fallbackAppName};
        searches << QStringList{QStringLiteral("--class"), fallbackAppName};
    }

    const QString fallbackExecutableBaseName = QFileInfo(fallbackExecutablePath).fileName();
    if (!fallbackExecutableBaseName.isEmpty()) {
        searches << QStringList{QStringLiteral("--class"), fallbackExecutableBaseName};
        searches << QStringList{QStringLiteral("--name"), fallbackExecutableBaseName};
    }

    return bestWindowIdForSearches(searches, onlyVisible);
}

bool activateWindowForServiceOrDesktop(const QString &serviceName,
                                       const QString &desktopFilePath,
                                       const QString &fallbackAppName,
                                       const QString &fallbackExecutablePath)
{
    const uint pid = serviceProcessId(serviceName);
    if (pid > 0) {
        QString windowId = bestWindowIdForSearches({QStringList{QStringLiteral("--pid"), QString::number(pid)}}, false);
        if (windowId.isEmpty()) {
            windowId = bestWindowIdForSearches({QStringList{QStringLiteral("--pid"), QString::number(pid)}});
        }
        if (activateWindow(windowId)) {
            return true;
        }
    }

    QString windowId = bestWindowIdForDesktopEntry(desktopFilePath, fallbackAppName, fallbackExecutablePath, false);
    if (windowId.isEmpty()) {
        windowId = bestWindowIdForDesktopEntry(desktopFilePath, fallbackAppName, fallbackExecutablePath);
    }

    return activateWindow(windowId);
}

bool moveWeatherWindowToRequestedPosition(int taskbarLeft, int taskbarTop)
{
    QString windowId = bestWindowIdForSearches({
        QStringList{QStringLiteral("--class"), QStringLiteral("deepin-weather")},
        QStringList{QStringLiteral("--class"), QStringLiteral("org.deepin.weather")},
        QStringList{QStringLiteral("--name"), QStringLiteral("org.deepin.weather")},
    });
    if (windowId.isEmpty()) {
        windowId = bestWindowIdForSearches({
            QStringList{QStringLiteral("--class"), QStringLiteral("deepin-weather")},
            QStringList{QStringLiteral("--class"), QStringLiteral("org.deepin.weather")},
            QStringList{QStringLiteral("--name"), QStringLiteral("org.deepin.weather")},
        }, false);
    }
    if (windowId.isEmpty()) {
        return false;
    }

    QRect geometry;
    if (!windowGeometry(windowId, &geometry)) {
        return false;
    }

    QScreen *screen = QGuiApplication::screenAt(geometry.center());
    if (!screen) {
        screen = QGuiApplication::screenAt(QPoint(taskbarLeft, taskbarTop));
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    const QRect availableGeometry = screen ? screen->availableGeometry()
                                           : QRect(taskbarLeft, geometry.y(), geometry.width(), geometry.height());
    int targetX = taskbarLeft;
    int targetY = geometry.y();
    const int maxX = availableGeometry.left() + qMax(0, availableGeometry.width() - geometry.width());
    const int maxY = availableGeometry.top() + qMax(0, availableGeometry.height() - geometry.height());

    if (targetY < availableGeometry.top() || targetY > maxY) {
        targetY = taskbarTop - geometry.height() - 10;
    }

    targetX = qBound(availableGeometry.left(), targetX, maxX);
    targetY = qBound(availableGeometry.top(), targetY, maxY);

    if (!moveWindow(windowId, targetX, targetY)) {
        return false;
    }

    return activateWindow(windowId);
}

MprisMetadataFallback metadataFallbackFromCommand(const QString &service)
{
    if (service.isEmpty()) {
        return {};
    }

    QProcess process;
    process.start(QStringLiteral("dbus-send"),
                  {
                      QStringLiteral("--session"),
                      QStringLiteral("--dest=%1").arg(service),
                      QStringLiteral("--print-reply"),
                      QStringLiteral("/org/mpris/MediaPlayer2"),
                      QStringLiteral("org.freedesktop.DBus.Properties.Get"),
                      QStringLiteral("string:org.mpris.MediaPlayer2.Player"),
                      QStringLiteral("string:Metadata"),
                  });
    if (!process.waitForFinished(800)) {
        process.kill();
        process.waitForFinished(200);
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QRegularExpression titlePattern(QStringLiteral(R"mpris(string "xesam:title"\s+variant\s+string "([^"]*)")mpris"),
                                          QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression artistPattern(QStringLiteral(R"mpris(string "xesam:artist"\s+variant\s+array \[\s+string "([^"]*)")mpris"),
                                           QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression artPattern(QStringLiteral(R"mpris(string "mpris:artUrl"\s+variant\s+string "([^"]*)")mpris"),
                                        QRegularExpression::DotMatchesEverythingOption);

    MprisMetadataFallback metadata;
    const QRegularExpressionMatch titleMatch = titlePattern.match(output);
    if (titleMatch.hasMatch()) {
        metadata.title = titleMatch.captured(1).trimmed();
    }

    const QRegularExpressionMatch artistMatch = artistPattern.match(output);
    if (artistMatch.hasMatch()) {
        metadata.artist = artistMatch.captured(1).trimmed();
    }

    const QRegularExpressionMatch artMatch = artPattern.match(output);
    if (artMatch.hasMatch()) {
        metadata.artUrl = artMatch.captured(1).trimmed();
    }

    return metadata;
}

int browserPenaltyForIdentity(const QString &identity)
{
    static const QRegularExpression browserPattern(QStringLiteral("(browser|chrome|chromium|firefox|edge|safari|浏览器)"),
                                                   QRegularExpression::CaseInsensitiveOption);
    return browserPattern.match(identity).hasMatch() ? -40 : 0;
}

bool isBrowserIdentity(const QString &identity)
{
    static const QRegularExpression browserPattern(QStringLiteral("(browser|chrome|chromium|firefox|edge|safari|浏览器)"),
                                                   QRegularExpression::CaseInsensitiveOption);
    return browserPattern.match(identity.trimmed()).hasMatch();
}

bool isBrowserDesktopId(const QString &desktopId)
{
    static const QRegularExpression browserPattern(QStringLiteral("(^|[._-])(browser|chrome|chromium|firefox|edge|brave|vivaldi|opera)([._-]|$)"),
                                                   QRegularExpression::CaseInsensitiveOption);
    QString normalizedDesktopId = desktopId.trimmed();
    if (normalizedDesktopId.endsWith(QStringLiteral(".desktop"))) {
        normalizedDesktopId.chop(QStringLiteral(".desktop").size());
    }

    return browserPattern.match(normalizedDesktopId).hasMatch();
}

bool isBrowserServiceName(const QString &serviceName)
{
    return isBrowserDesktopId(serviceName);
}

bool isDedicatedMusicIdentity(const QString &identity)
{
    const QString trimmedIdentity = identity.trimmed();
    return !trimmedIdentity.isEmpty() && !isBrowserIdentity(trimmedIdentity);
}

QString iconNameForMusicAppName(const QString &appName)
{
    const QString normalizedName = appName.trimmed().toLower();
    if (normalizedName.isEmpty()) {
        return {};
    }

    if (normalizedName.contains(QStringLiteral("网易云"))
        || normalizedName.contains(QStringLiteral("网易"))
        || normalizedName.contains(QStringLiteral("云音乐"))
        || normalizedName.contains(QStringLiteral("netease"))) {
        return QStringLiteral("netease-cloud-music");
    }

    if (normalizedName.contains(QStringLiteral("deepin music"))
        || normalizedName.contains(QStringLiteral("deepin-music"))
        || normalizedName.contains(QStringLiteral("com.deepin.music"))
        || normalizedName.contains(QStringLiteral("deepin音乐"))
        || normalizedName == QStringLiteral("音乐")) {
        return QStringLiteral("deepin-music");
    }

    return {};
}

QUrl stableMusicArtSource(const QString &artUrl)
{
    const QUrl sourceUrl(artUrl);
    if (!sourceUrl.isValid() || sourceUrl.isEmpty()) {
        return {};
    }

    if (!sourceUrl.isLocalFile()) {
        return sourceUrl;
    }

    const QString sourcePath = sourceUrl.toLocalFile();
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable()) {
        return sourceUrl;
    }

    QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        cacheRoot = QDir::home().filePath(QStringLiteral(".cache/dde-shell"));
    }

    QDir cacheDirectory(QDir(cacheRoot).filePath(QStringLiteral("fashion-left-plugin/music-art")));
    if (!cacheDirectory.exists() && !cacheDirectory.mkpath(QStringLiteral("."))) {
        return sourceUrl;
    }

    const QByteArray imageFormat = QImageReader::imageFormat(sourcePath).toLower();
    const QString suffix = !imageFormat.isEmpty()
        ? QString::fromLatin1(imageFormat)
        : sourceInfo.suffix().trimmed().toLower();

    QByteArray hashSeed = sourcePath.toUtf8();
    hashSeed += QByteArray::number(sourceInfo.size());
    hashSeed += QByteArray::number(sourceInfo.lastModified().toMSecsSinceEpoch());
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(hashSeed, QCryptographicHash::Sha1).toHex());
    const QString cacheFileName = suffix.isEmpty() ? hash : QStringLiteral("%1.%2").arg(hash, suffix);
    const QString cachePath = cacheDirectory.filePath(cacheFileName);

    if (!QFileInfo::exists(cachePath)) {
        QFile::remove(cachePath);
        if (!QFile::copy(sourcePath, cachePath)) {
            return sourceUrl;
        }
    }

    return QUrl::fromLocalFile(cachePath);
}

QString joinMusicSubtitleParts(const QStringList &parts)
{
    QStringList filteredParts;
    for (const QString &part : parts) {
        const QString trimmedPart = part.trimmed();
        if (!trimmedPart.isEmpty() && !filteredParts.contains(trimmedPart)) {
            filteredParts << trimmedPart;
        }
    }

    return filteredParts.join(QStringLiteral(" · "));
}

QString desktopEntryFromMprisService(const QString &serviceName)
{
    constexpr auto prefix = "org.mpris.MediaPlayer2.";
    if (!serviceName.startsWith(QLatin1String(prefix))) {
        return {};
    }

    QString desktopId = serviceName.mid(static_cast<int>(strlen(prefix)));
    const qsizetype instanceSeparatorIndex = desktopId.indexOf(QStringLiteral(".instance"));
    if (instanceSeparatorIndex > 0) {
        desktopId.truncate(instanceSeparatorIndex);
    }

    const qsizetype nestedServiceSeparatorIndex = desktopId.indexOf(QLatin1Char('.'));
    if (nestedServiceSeparatorIndex > 0) {
        desktopId.truncate(nestedServiceSeparatorIndex);
    }

    return desktopId.trimmed();
}

QString iconPathForName(const QString &iconName)
{
    if (iconName.isEmpty()) {
        return {};
    }

    static QHash<QString, QString> cache;
    const auto cachedResult = cache.constFind(iconName);
    if (cachedResult != cache.cend()) {
        return cachedResult.value();
    }

    const QFileInfo iconFileInfo(iconName);
    if (iconFileInfo.exists() && iconFileInfo.isFile()) {
        const QString absolutePath = iconFileInfo.absoluteFilePath();
        cache.insert(iconName, absolutePath);
        return absolutePath;
    }

    const QString preferredIconPath = firstExistingPath({
        QStringLiteral("/usr/share/icons/Win11/apps/scalable/%1.svg").arg(iconName),
        QStringLiteral("/usr/share/icons/Win11/mimes/scalable/%1.svg").arg(iconName),
        QStringLiteral("/usr/share/icons/Win11/mimes/22/%1.svg").arg(iconName),
        QStringLiteral("/usr/share/icons/Win11/mimes/16/%1.svg").arg(iconName),
        QStringLiteral("/usr/share/icons/hicolor/scalable/apps/%1.svg").arg(iconName),
        QStringLiteral("/var/lib/linglong/entries/share/icons/hicolor/scalable/apps/%1.svg").arg(iconName),
        QStringLiteral("/usr/share/pixmaps/%1.png").arg(iconName),
        QStringLiteral("/usr/share/pixmaps/%1.svg").arg(iconName),
    });
    if (!preferredIconPath.isEmpty()) {
        cache.insert(iconName, preferredIconPath);
        return preferredIconPath;
    }

    const QStringList searchRoots = {
        QDir::home().filePath(QStringLiteral(".local/share/icons")),
        QStringLiteral("/usr/share/icons"),
        QStringLiteral("/usr/share/pixmaps"),
    };
    const QStringList nameFilters = {
        iconName,
        iconName + QStringLiteral(".png"),
        iconName + QStringLiteral(".svg"),
        iconName + QStringLiteral(".xpm"),
    };

    for (const QString &rootPath : searchRoots) {
        if (!QFileInfo(rootPath).exists()) {
            continue;
        }

        QDirIterator iterator(rootPath,
                              nameFilters,
                              QDir::Files | QDir::Readable,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString path = iterator.next();
            cache.insert(iconName, path);
            return path;
        }
    }

    cache.insert(iconName, QString());
    return {};
}

MusicSnapshot currentMusicSnapshot(const QString &previousService)
{
    MusicSnapshot bestSnapshot;

    QDBusConnectionInterface *connectionInterface = QDBusConnection::sessionBus().interface();
    if (!connectionInterface) {
        return bestSnapshot;
    }

    const QDBusReply<QStringList> namesReply = connectionInterface->registeredServiceNames();
    if (!namesReply.isValid()) {
        return bestSnapshot;
    }

    const QStringList serviceNames = namesReply.value();
    for (const QString &serviceName : serviceNames) {
        if (!serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2."))) {
            continue;
        }

        const QVariantMap playerProperties = dbusProperties(serviceName, QLatin1String(MprisPlayerInterface));
        if (playerProperties.isEmpty()) {
            continue;
        }

        const QVariantMap rootProperties = dbusProperties(serviceName, QLatin1String(MprisRootInterface));
        const QString identity = stringFromDBusValue(rootProperties.value(QStringLiteral("Identity")));
        const QString playbackStatus = stringFromDBusValue(playerProperties.value(QStringLiteral("PlaybackStatus")));
        const QVariantMap metadata = mapFromDBusValue(playerProperties.value(QStringLiteral("Metadata")));

        MusicSnapshot snapshot;
        snapshot.service = serviceName;
        snapshot.desktopEntry = stringFromDBusValue(rootProperties.value(QStringLiteral("DesktopEntry")));
        if (snapshot.desktopEntry.isEmpty()) {
            snapshot.desktopEntry = desktopEntryFromMprisService(serviceName);
        }
        snapshot.appName = identity;
        snapshot.available = true;
        snapshot.playing = playbackStatus == QStringLiteral("Playing");
        snapshot.canRaise = boolFromDBusValue(rootProperties.value(QStringLiteral("CanRaise")));
        snapshot.canGoPrevious = boolFromDBusValue(playerProperties.value(QStringLiteral("CanGoPrevious")));
        snapshot.canGoNext = boolFromDBusValue(playerProperties.value(QStringLiteral("CanGoNext")));
        snapshot.canTogglePlayback = boolFromDBusValue(playerProperties.value(QStringLiteral("CanPause")))
            || boolFromDBusValue(playerProperties.value(QStringLiteral("CanPlay")));

        QString title = stringFromDBusValue(metadata.value(QStringLiteral("xesam:title")));
        QString subtitle = joinMusicSubtitleParts(stringListFromDBusValue(metadata.value(QStringLiteral("xesam:artist"))));
        QString artUrl = stringFromDBusValue(metadata.value(QStringLiteral("mpris:artUrl")));
        if (title.isEmpty() || artUrl.isEmpty()) {
            const MprisMetadataFallback fallbackMetadata = metadataFallbackFromCommand(serviceName);
            if (title.isEmpty()) {
                title = fallbackMetadata.title;
            }
            if (artUrl.isEmpty()) {
                artUrl = fallbackMetadata.artUrl;
            }
        }

        snapshot.title = title;
        snapshot.subtitle = subtitle;

        if (!artUrl.isEmpty()) {
            snapshot.artSource = stableMusicArtSource(artUrl);
        }

        snapshot.score = 0;
        if (playbackStatus == QStringLiteral("Playing")) {
            snapshot.score += 300;
        } else if (playbackStatus == QStringLiteral("Paused")) {
            snapshot.score += 200;
        } else {
            snapshot.score += 100;
        }

        if (!title.isEmpty()) {
            snapshot.score += 30;
        }
        if (snapshot.artSource.isValid()) {
            snapshot.score += 15;
        }
        if (boolFromDBusValue(playerProperties.value(QStringLiteral("CanControl")))) {
            snapshot.score += 10;
        }
        if (serviceName == previousService) {
            snapshot.score += 5;
        }

        const bool dedicatedMusicIdentity = isDedicatedMusicIdentity(identity);
        const bool browserShellService = (isBrowserDesktopId(snapshot.desktopEntry) || isBrowserServiceName(serviceName))
            && !dedicatedMusicIdentity;
        if (browserShellService) {
            continue;
        }

        if (snapshot.canGoPrevious || snapshot.canGoNext) {
            snapshot.score += 80;
        }
        if (dedicatedMusicIdentity) {
            snapshot.score += 100;
        }
        snapshot.score += browserPenaltyForIdentity(identity);

        if (!bestSnapshot.available || snapshot.score > bestSnapshot.score) {
            bestSnapshot = snapshot;
        }
    }

    if (bestSnapshot.available) {
        const MprisMetadataFallback fallbackMetadata = metadataFallbackFromCommand(bestSnapshot.service);
        if (bestSnapshot.title.isEmpty()) {
            bestSnapshot.title = fallbackMetadata.title;
        }
        if (bestSnapshot.subtitle.isEmpty()) {
            bestSnapshot.subtitle = fallbackMetadata.artist;
        }
        if (!bestSnapshot.artSource.isValid() && !fallbackMetadata.artUrl.isEmpty()) {
            bestSnapshot.artSource = stableMusicArtSource(fallbackMetadata.artUrl);
        }
    }

    return bestSnapshot;
}

QLibrary &weatherDockPluginLibrary()
{
    static QLibrary library(WeatherDockPluginPath);
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        library.load();
    }

    return library;
}

WeatherCodeToDescriptionFunction weatherCodeToDescriptionFunction()
{
    static const auto function = reinterpret_cast<WeatherCodeToDescriptionFunction>(
        weatherDockPluginLibrary().resolve("_ZN17WeatherController24weatherCodeToDescriptionEi"));
    return function;
}

WeatherCodeToIconNameFunction weatherCodeToIconNameFunction()
{
    static const auto function = reinterpret_cast<WeatherCodeToIconNameFunction>(
        weatherDockPluginLibrary().resolve("_ZN17WeatherController21weatherCodeToIconNameEib"));
    return function;
}

QString weatherAssetPath(const QString &assetName)
{
    return firstExistingPath({
        QStringLiteral("/usr/share/icons/Win11/status/32/%1.svg").arg(assetName),
        QStringLiteral("/usr/share/icons/Win11/status/16/%1.svg").arg(assetName),
        QStringLiteral("/usr/share/icons/Adwaita/symbolic/status/%1-symbolic.svg").arg(assetName),
        WeatherAppIconPath,
    });
}

QString weatherAssetNameFor(const QString &iconName, bool isDay)
{
    const QString normalizedIconName = iconName.trimmed().toLower().replace(QLatin1Char('-'), QLatin1Char('_'));

    if (normalizedIconName.contains(QStringLiteral("tornado"))) {
        return QStringLiteral("weather-storm-tornado");
    }

    if (normalizedIconName.contains(QStringLiteral("thunder"))
        || normalizedIconName.contains(QStringLiteral("storm"))) {
        return isDay ? QStringLiteral("weather-storm") : QStringLiteral("weather-storm-night");
    }

    if (normalizedIconName.contains(QStringLiteral("hail"))) {
        return QStringLiteral("weather-hail");
    }

    if (normalizedIconName.contains(QStringLiteral("freezing"))
        || normalizedIconName.contains(QStringLiteral("sleet"))) {
        return QStringLiteral("weather-freezing-rain");
    }

    if ((normalizedIconName.contains(QStringLiteral("snow")) && normalizedIconName.contains(QStringLiteral("rain")))
        || normalizedIconName.contains(QStringLiteral("rain_snow"))
        || normalizedIconName.contains(QStringLiteral("snow_rain"))) {
        return QStringLiteral("weather-snow-rain");
    }

    if (normalizedIconName.contains(QStringLiteral("snow"))) {
        return isDay ? QStringLiteral("weather-snow") : QStringLiteral("weather-snow-night");
    }

    if (normalizedIconName.contains(QStringLiteral("fog"))
        || normalizedIconName.contains(QStringLiteral("mist"))
        || normalizedIconName.contains(QStringLiteral("haze"))
        || normalizedIconName.contains(QStringLiteral("smog"))) {
        return QStringLiteral("weather-fog");
    }

    if (normalizedIconName.contains(QStringLiteral("overcast"))) {
        return isDay ? QStringLiteral("weather-overcast") : QStringLiteral("weather-overcast-night");
    }

    if (normalizedIconName.contains(QStringLiteral("wind"))) {
        return QStringLiteral("weather-windy");
    }

    if (normalizedIconName.contains(QStringLiteral("few"))
        || normalizedIconName.contains(QStringLiteral("partly"))) {
        return isDay ? QStringLiteral("weather-few-clouds") : QStringLiteral("weather-few-clouds-night");
    }

    if (normalizedIconName.contains(QStringLiteral("cloud"))
        || normalizedIconName.contains(QStringLiteral("cloudy"))
        || normalizedIconName.contains(QStringLiteral("many_clouds"))) {
        return isDay ? QStringLiteral("weather-clouds") : QStringLiteral("weather-clouds-night");
    }

    if (normalizedIconName.contains(QStringLiteral("drizzle"))
        || normalizedIconName.contains(QStringLiteral("shower"))
        || normalizedIconName.contains(QStringLiteral("showers"))
        || normalizedIconName.contains(QStringLiteral("rain"))) {
        return isDay ? QStringLiteral("weather-showers-day") : QStringLiteral("weather-showers-night");
    }

    if (normalizedIconName.contains(QStringLiteral("clear"))
        || normalizedIconName.contains(QStringLiteral("sun"))
        || normalizedIconName.contains(QStringLiteral("sunny"))
        || normalizedIconName.contains(QStringLiteral("fine"))) {
        return isDay ? QStringLiteral("weather-clear") : QStringLiteral("weather-clear-night");
    }

    return QStringLiteral("weather-none-available");
}

QString deepinWeatherDescription(int weatherCode)
{
    const auto function = weatherCodeToDescriptionFunction();
    if (!function) {
        return {};
    }

    return function(weatherCode).trimmed();
}

QString deepinWeatherIconName(int weatherCode, bool isDay)
{
    const auto function = weatherCodeToIconNameFunction();
    if (!function) {
        return {};
    }

    return function(weatherCode, isDay).trimmed();
}

} // namespace

FashionLeftPluginProvider::FashionLeftPluginProvider(QObject *parent)
    : QObject(parent)
{
    auto clockTimer = new QTimer(this);
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshClock);
    clockTimer->start();

    auto notificationTimer = new QTimer(this);
    notificationTimer->setInterval(15000);
    connect(notificationTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshNotificationCount);
    notificationTimer->start();

    auto mailTimer = new QTimer(this);
    mailTimer->setInterval(15000);
    connect(mailTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshMailState);
    mailTimer->start();

    auto musicTimer = new QTimer(this);
    musicTimer->setInterval(1000);
    connect(musicTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshMusicState);
    musicTimer->start();

    auto statsTimer = new QTimer(this);
    statsTimer->setInterval(1000);
    connect(statsTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshSystemStats);
    statsTimer->start();

    auto weatherTimer = new QTimer(this);
    weatherTimer->setInterval(300000);
    connect(weatherTimer, &QTimer::timeout, this, &FashionLeftPluginProvider::refreshWeather);
    weatherTimer->start();

    m_weatherWatcher = new QFileSystemWatcher(this);
    connect(m_weatherWatcher, &QFileSystemWatcher::fileChanged, this, [this] {
        refreshWeather();
    });
    connect(m_weatherWatcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        refreshWeather();
    });

    QDBusConnection::sessionBus().connect(NotificationService,
                                          NotificationPath,
                                          NotificationInterface,
                                          QStringLiteral("RecordCountChanged"),
                                          this,
                                          SLOT(onNotificationCountChanged(uint)));

    refreshClock();
    refreshNotificationCount();
    refreshMailClient();
    refreshMailState();
    refreshMusicState();
    refreshSystemStats();
    refreshWeather();
}

QString FashionLeftPluginProvider::timeText() const
{
    return m_timeText;
}

QString FashionLeftPluginProvider::dateText() const
{
    return m_dateText;
}

int FashionLeftPluginProvider::notificationCount() const
{
    return m_notificationCount;
}

QString FashionLeftPluginProvider::notificationCountText() const
{
    if (m_notificationCount > 99) {
        return QStringLiteral("99+");
    }
    return QString::number(m_notificationCount);
}

int FashionLeftPluginProvider::mailUnreadCount() const
{
    return m_mailUnreadCount;
}

QString FashionLeftPluginProvider::mailUnreadCountText() const
{
    return QString::number(m_mailUnreadCount);
}

QString FashionLeftPluginProvider::mailSummaryText() const
{
    return m_mailSummaryText;
}

QString FashionLeftPluginProvider::mailIconName() const
{
    return m_mailIconName;
}

QString FashionLeftPluginProvider::mailClientName() const
{
    return m_mailClientName;
}

bool FashionLeftPluginProvider::musicAvailable() const
{
    return m_musicAvailable;
}

QString FashionLeftPluginProvider::musicTitleText() const
{
    return m_musicTitleText;
}

QString FashionLeftPluginProvider::musicSubtitleText() const
{
    return m_musicSubtitleText;
}

QString FashionLeftPluginProvider::musicAppName() const
{
    return m_musicAppName;
}

QUrl FashionLeftPluginProvider::musicArtSource() const
{
    return m_musicArtSource;
}

QString FashionLeftPluginProvider::musicPlayerIconName() const
{
    return m_musicPlayerIconName;
}

QUrl FashionLeftPluginProvider::musicPlayerIconSource() const
{
    return m_musicPlayerIconSource;
}

bool FashionLeftPluginProvider::musicHasArt() const
{
    return m_musicArtSource.isValid() && !m_musicArtSource.isEmpty();
}

bool FashionLeftPluginProvider::musicPlaying() const
{
    return m_musicPlaying;
}

bool FashionLeftPluginProvider::musicCanGoPrevious() const
{
    return m_musicCanGoPrevious;
}

bool FashionLeftPluginProvider::musicCanGoNext() const
{
    return m_musicCanGoNext;
}

bool FashionLeftPluginProvider::musicCanTogglePlayback() const
{
    return m_musicCanTogglePlayback;
}

int FashionLeftPluginProvider::cpuUsage() const
{
    return m_cpuUsage;
}

int FashionLeftPluginProvider::memoryUsage() const
{
    return m_memoryUsage;
}

QString FashionLeftPluginProvider::downloadSpeedText() const
{
    return m_downloadSpeedText;
}

QString FashionLeftPluginProvider::uploadSpeedText() const
{
    return m_uploadSpeedText;
}

QString FashionLeftPluginProvider::weatherCityText() const
{
    return m_weatherCityText;
}

QString FashionLeftPluginProvider::weatherTemperatureText() const
{
    return m_weatherTemperatureText;
}

QString FashionLeftPluginProvider::weatherSummaryText() const
{
    return m_weatherSummaryText;
}

QUrl FashionLeftPluginProvider::weatherIconSource() const
{
    return m_weatherIconSource;
}

QUrl FashionLeftPluginProvider::messageIconSource() const
{
    return QUrl::fromLocalFile(MessageIconPath);
}

void FashionLeftPluginProvider::openWeatherPage()
{
    const QString desktopFilePath = locateDesktopFile(QStringLiteral("org.deepin.weather.desktop"));
    if (!desktopFilePath.isEmpty() && launchDesktopEntry(desktopFilePath)) {
        return;
    }

    if (launchCommand(QStringLiteral("deepin-weather"))) {
        return;
    }

    launchCommand(QStringLiteral("gtk-launch"), {QStringLiteral("org.deepin.weather")});
}

void FashionLeftPluginProvider::openWeatherPopup(int taskbarLeft, int taskbarTop, int activationX, int activationY)
{
    const int popupLeft = qMax(0, taskbarLeft);
    const int popupTop = qMax(0, taskbarTop);
    const int popupX = qMax(0, activationX);
    const int popupY = qMax(0, activationY);
    const auto scheduleWeatherWindowPlacement = [this, popupLeft, popupTop] {
        moveWeatherWindowToRequestedPosition(popupLeft, popupTop);
        for (const int delay : {0, 16, 48, 120}) {
            QTimer::singleShot(delay, this, [popupLeft, popupTop] {
                moveWeatherWindowToRequestedPosition(popupLeft, popupTop);
            });
        }
    };

    if (activateStatusNotifierItem(QStringLiteral("org.deepin.weather"), popupX, popupY)) {
        scheduleWeatherWindowPlacement();
        return;
    }

    openWeatherPage();
    QTimer::singleShot(700, this, [this, popupLeft, popupTop, popupX, popupY, scheduleWeatherWindowPlacement] {
        if (activateStatusNotifierItem(QStringLiteral("org.deepin.weather"), popupX, popupY)) {
            scheduleWeatherWindowPlacement();
        }
    });
}

void FashionLeftPluginProvider::openMailClient()
{
    refreshMailClient();

    if (!m_mailDesktopFilePath.isEmpty()
        && launchCommand(QStringLiteral("gio"), {QStringLiteral("launch"), m_mailDesktopFilePath})) {
        return;
    }

    if (!m_mailDesktopId.isEmpty()) {
        const QString desktopFilePath = locateDesktopFile(m_mailDesktopId);
        if (!desktopFilePath.isEmpty()
            && launchCommand(QStringLiteral("gio"), {QStringLiteral("launch"), desktopFilePath})) {
            return;
        }
    }

    if (launchCommand(QStringLiteral("xdg-open"), {QStringLiteral("mailto:")})) {
        return;
    }

    launchCommand(QStringLiteral("ll-cli"),
                  {QStringLiteral("run"),
                   QStringLiteral("org.deepin.mail"),
                   QStringLiteral("--"),
                   QStringLiteral("/opt/apps/org.deepin.mail/files/bin/deepin-mail")});
}

void FashionLeftPluginProvider::openNotificationPage()
{
    showControlCenterPage(QStringLiteral("system/notification"));
}

void FashionLeftPluginProvider::openMusicPlayer()
{
    if (m_musicService.isEmpty()) {
        return;
    }

    QString desktopId = m_musicDesktopEntry.trimmed();
    if (!desktopId.isEmpty() && !desktopId.endsWith(QStringLiteral(".desktop"))) {
        desktopId += QStringLiteral(".desktop");
    }

    const QString executablePath = executablePathForService(m_musicService);
    QString desktopFilePath;
    if (!desktopId.isEmpty()) {
        desktopFilePath = locateDesktopFile(desktopId);
    }
    if (desktopFilePath.isEmpty()) {
        desktopFilePath = locateDesktopFileByExecutable(executablePath);
    }

    const auto scheduleMusicWindowActivation = [this, desktopFilePath, executablePath] {
        QTimer::singleShot(120, this, [this, desktopFilePath, executablePath] {
            activateWindowForServiceOrDesktop(m_musicService, desktopFilePath, m_musicAppName, executablePath);
        });
        QTimer::singleShot(420, this, [this, desktopFilePath, executablePath] {
            activateWindowForServiceOrDesktop(m_musicService, desktopFilePath, m_musicAppName, executablePath);
        });
    };

    if (activateWindowForServiceOrDesktop(m_musicService, desktopFilePath, m_musicAppName, executablePath)) {
        return;
    }

    QDBusInterface rootInterface(m_musicService,
                                 MprisPath,
                                 MprisRootInterface,
                                 QDBusConnection::sessionBus());
    if (rootInterface.isValid()) {
        const QDBusMessage raiseReply = rootInterface.call(QStringLiteral("Raise"));
        if (raiseReply.type() != QDBusMessage::ErrorMessage) {
            scheduleMusicWindowActivation();
            return;
        }
    }

    if (!desktopFilePath.isEmpty() && launchDesktopEntry(desktopFilePath)) {
        scheduleMusicWindowActivation();
        return;
    }

    if (!executablePath.isEmpty() && launchCommand(executablePath)) {
        scheduleMusicWindowActivation();
        return;
    }

    const QString desktopBaseId = QFileInfo(desktopId).completeBaseName();
    if (!desktopBaseId.isEmpty() && launchCommand(QStringLiteral("gtk-launch"), {desktopBaseId})) {
        scheduleMusicWindowActivation();
        return;
    }

    if (!desktopId.isEmpty() && launchCommand(QStringLiteral("gtk-launch"), {desktopId})) {
        scheduleMusicWindowActivation();
    }
}

void FashionLeftPluginProvider::playPreviousTrack()
{
    if (m_musicService.isEmpty() || !m_musicCanGoPrevious) {
        return;
    }

    QDBusInterface playerInterface(m_musicService,
                                   MprisPath,
                                   MprisPlayerInterface,
                                   QDBusConnection::sessionBus());
    playerInterface.call(QStringLiteral("Previous"));
    refreshMusicState();
}

void FashionLeftPluginProvider::toggleMusicPlayback()
{
    if (m_musicService.isEmpty() || !m_musicCanTogglePlayback) {
        return;
    }

    QDBusInterface playerInterface(m_musicService,
                                   MprisPath,
                                   MprisPlayerInterface,
                                   QDBusConnection::sessionBus());
    playerInterface.call(QStringLiteral("PlayPause"));
    refreshMusicState();
}

void FashionLeftPluginProvider::playNextTrack()
{
    if (m_musicService.isEmpty() || !m_musicCanGoNext) {
        return;
    }

    QDBusInterface playerInterface(m_musicService,
                                   MprisPath,
                                   MprisPlayerInterface,
                                   QDBusConnection::sessionBus());
    playerInterface.call(QStringLiteral("Next"));
    refreshMusicState();
}

void FashionLeftPluginProvider::openSystemMonitorPage()
{
    const QString desktopFilePath = locateDesktopFile(QStringLiteral("deepin-system-monitor.desktop"));
    const QString monitorName = localizedDesktopEntryValue(desktopFilePath, QStringLiteral("Name"));
    const QString monitorExecutable = desktopCommandExecutable(desktopFilePath);
    const auto requestRaiseWindow = [this, desktopFilePath, monitorName, monitorExecutable] {
        QTimer::singleShot(150, this, [desktopFilePath, monitorName, monitorExecutable] {
            callDBusMethod(QLatin1String(SystemMonitorMainService),
                           QLatin1String(SystemMonitorMainPath),
                           QLatin1String(SystemMonitorMainInterface),
                           QStringLiteral("slotRaiseWindow"));
            activateWindowForServiceOrDesktop(QString(), desktopFilePath, monitorName, monitorExecutable);
        });
        QTimer::singleShot(500, this, [desktopFilePath, monitorName, monitorExecutable] {
            callDBusMethod(QLatin1String(SystemMonitorMainService),
                           QLatin1String(SystemMonitorMainPath),
                           QLatin1String(SystemMonitorMainInterface),
                           QStringLiteral("slotRaiseWindow"));
            activateWindowForServiceOrDesktop(QString(), desktopFilePath, monitorName, monitorExecutable);
        });
    };

    if (activateWindowForServiceOrDesktop(QString(), desktopFilePath, monitorName, monitorExecutable)) {
        return;
    }

    if (callDBusMethod(QLatin1String(SystemMonitorMainService),
                       QLatin1String(SystemMonitorMainPath),
                       QLatin1String(SystemMonitorMainInterface),
                       QStringLiteral("slotRaiseWindow"))) {
        requestRaiseWindow();
        return;
    }

    if (callDBusMethod(QLatin1String(SystemMonitorServerService),
                       QLatin1String(SystemMonitorServerPath),
                       QLatin1String(SystemMonitorServerInterface),
                       QStringLiteral("showDeepinSystemMoniter"))) {
        requestRaiseWindow();
        return;
    }

    if (callDBusMethod(QLatin1String(SystemMonitorService),
                       QLatin1String(SystemMonitorPath),
                       QLatin1String(SystemMonitorInterface),
                       QStringLiteral("showDeepinSystemMoniter"))) {
        requestRaiseWindow();
        return;
    }

    if (!desktopFilePath.isEmpty() && launchDesktopEntry(desktopFilePath)) {
        requestRaiseWindow();
        return;
    }

    launchCommand(QStringLiteral("deepin-system-monitor"));
    requestRaiseWindow();
}

void FashionLeftPluginProvider::refreshClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString nextTimeText = now.time().toString(QStringLiteral("HH:mm"));
    const QString nextDateText = QLocale().toString(now.date(), QStringLiteral("M/d dddd"));

    if (m_timeText == nextTimeText && m_dateText == nextDateText) {
        return;
    }

    m_timeText = nextTimeText;
    m_dateText = nextDateText;
    emit clockChanged();
}

void FashionLeftPluginProvider::refreshNotificationCount()
{
    QDBusInterface notificationInterface(NotificationService,
                                         NotificationPath,
                                         NotificationInterface,
                                         QDBusConnection::sessionBus());
    if (!notificationInterface.isValid()) {
        return;
    }

    const QDBusReply<uint> reply = notificationInterface.call(QStringLiteral("recordCount"));
    if (!reply.isValid()) {
        return;
    }

    onNotificationCountChanged(reply.value());
}

void FashionLeftPluginProvider::refreshMailState()
{
    QDBusInterface mailInterface(MailService,
                                 MailPath,
                                 MailInterface,
                                 QDBusConnection::sessionBus());

    int nextMailUnreadCount = 0;
    QString nextMailSummaryText = QStringLiteral("邮箱信息不可用");
    bool hasUnreadData = false;

    if (mailInterface.isValid()) {
        const QDBusReply<QString> accountsReply = mailInterface.call(QStringLiteral("GetAccounts"));
        const QStringList accountIds = accountsReply.isValid()
            ? mailAccountIdsFromJson(accountsReply.value())
            : QStringList();

        if (accountIds.isEmpty()) {
            nextMailSummaryText = QStringLiteral("未配置邮箱账户");
        } else {
            for (const QString &accountId : accountIds) {
                const QDBusReply<QString> unreadReply = mailInterface.call(QStringLiteral("GetUnread"), accountId);
                if (!unreadReply.isValid()) {
                    continue;
                }

                bool unreadOk = false;
                const int unreadCount = unreadCountFromJson(unreadReply.value(), &unreadOk);
                if (!unreadOk) {
                    continue;
                }

                nextMailUnreadCount += unreadCount;
                hasUnreadData = true;
            }

            if (hasUnreadData) {
                nextMailSummaryText = nextMailUnreadCount > 0
                    ? QStringLiteral("收件箱有 %1 封未读邮件").arg(nextMailUnreadCount)
                    : QStringLiteral("暂时没有未读邮件");
            }
        }
    }

    if (m_mailUnreadCount == nextMailUnreadCount && m_mailSummaryText == nextMailSummaryText) {
        return;
    }

    m_mailUnreadCount = nextMailUnreadCount;
    m_mailSummaryText = nextMailSummaryText;
    emit mailStateChanged();
}

void FashionLeftPluginProvider::refreshMusicState()
{
    const MusicSnapshot snapshot = currentMusicSnapshot(m_musicService);

    QString nextDesktopEntry = snapshot.desktopEntry.trimmed();
    if (!nextDesktopEntry.isEmpty() && !nextDesktopEntry.endsWith(QStringLiteral(".desktop"))) {
        nextDesktopEntry += QStringLiteral(".desktop");
    }

    const QString nextDesktopFilePath = locateDesktopFile(nextDesktopEntry);
    QString nextMusicAppName = snapshot.appName;
    if (nextMusicAppName.isEmpty()) {
        nextMusicAppName = localizedDesktopEntryValue(nextDesktopFilePath, QStringLiteral("Name"));
    }
    const QString nextMusicPlayerIconName = musicPlayerIconNameForDesktopEntry(nextDesktopEntry,
                                                                               nextMusicAppName,
                                                                               snapshot.service);
    const QUrl nextMusicPlayerIconSource = iconSourceForName(nextMusicPlayerIconName);

    if (m_musicService == snapshot.service
        && m_musicDesktopEntry == nextDesktopEntry
        && m_musicTitleText == snapshot.title
        && m_musicSubtitleText == snapshot.subtitle
        && m_musicAppName == nextMusicAppName
        && m_musicArtSource == snapshot.artSource
        && m_musicPlayerIconName == nextMusicPlayerIconName
        && m_musicPlayerIconSource == nextMusicPlayerIconSource
        && m_musicAvailable == snapshot.available
        && m_musicPlaying == snapshot.playing
        && m_musicCanRaise == snapshot.canRaise
        && m_musicCanGoPrevious == snapshot.canGoPrevious
        && m_musicCanGoNext == snapshot.canGoNext
        && m_musicCanTogglePlayback == snapshot.canTogglePlayback) {
        return;
    }

    m_musicService = snapshot.service;
    m_musicDesktopEntry = nextDesktopEntry;
    m_musicTitleText = snapshot.title;
    m_musicSubtitleText = snapshot.subtitle;
    m_musicAppName = nextMusicAppName;
    m_musicArtSource = snapshot.artSource;
    m_musicPlayerIconName = nextMusicPlayerIconName;
    m_musicPlayerIconSource = nextMusicPlayerIconSource;
    m_musicAvailable = snapshot.available;
    m_musicPlaying = snapshot.playing;
    m_musicCanRaise = snapshot.canRaise;
    m_musicCanGoPrevious = snapshot.canGoPrevious;
    m_musicCanGoNext = snapshot.canGoNext;
    m_musicCanTogglePlayback = snapshot.canTogglePlayback;
    emit musicStateChanged();
}

void FashionLeftPluginProvider::refreshSystemStats()
{
    int nextCpuUsage = m_cpuUsage;
    quint64 totalCpuTime = 0;
    quint64 idleCpuTime = 0;
    if (readCpuTimes(&totalCpuTime, &idleCpuTime)) {
        if (m_previousCpuTotalTime > 0 && totalCpuTime > m_previousCpuTotalTime) {
            const quint64 totalDelta = totalCpuTime - m_previousCpuTotalTime;
            const quint64 idleDelta = idleCpuTime - m_previousCpuIdleTime;
            const double busyRatio = totalDelta > 0
                ? (static_cast<double>(totalDelta - qMin(idleDelta, totalDelta)) * 100.0 / totalDelta)
                : 0.0;
            nextCpuUsage = qBound(0, qRound(busyRatio), 100);
        }

        m_previousCpuTotalTime = totalCpuTime;
        m_previousCpuIdleTime = idleCpuTime;
    }

    int nextMemoryUsage = systemMemoryUsagePercent();
    querySystemMonitorUsage("getMemoryUsage", &nextMemoryUsage);

    const QStringList activeInterfaces = preferredNetworkInterfaces();
    const quint64 receiveBytes = totalInterfaceBytes(true, activeInterfaces);
    const quint64 transmitBytes = totalInterfaceBytes(false, activeInterfaces);
    const quint64 aggregateReceiveBytes = activeInterfaces.isEmpty() ? receiveBytes : totalInterfaceBytes(true, {});
    const quint64 aggregateTransmitBytes = activeInterfaces.isEmpty() ? transmitBytes : totalInterfaceBytes(false, {});

    QString nextDownloadSpeed = m_downloadSpeedText;
    QString nextUploadSpeed = m_uploadSpeedText;
    if (m_networkSampleTimer.isValid()) {
        const double elapsedSeconds = qMax(0.001, m_networkSampleTimer.restart() / 1000.0);
        quint64 receiveDelta = receiveBytes >= m_previousReceiveBytes
            ? (receiveBytes - m_previousReceiveBytes)
            : 0;
        quint64 transmitDelta = transmitBytes >= m_previousTransmitBytes
            ? (transmitBytes - m_previousTransmitBytes)
            : 0;
        const quint64 aggregateReceiveDelta = aggregateReceiveBytes >= m_previousAggregateReceiveBytes
            ? (aggregateReceiveBytes - m_previousAggregateReceiveBytes)
            : 0;
        const quint64 aggregateTransmitDelta = aggregateTransmitBytes >= m_previousAggregateTransmitBytes
            ? (aggregateTransmitBytes - m_previousAggregateTransmitBytes)
            : 0;

        // Some environments route user traffic through overlay/tunnel interfaces.
        // Fall back to total active traffic when the preferred interface delta stays at zero.
        if (receiveDelta == 0 && aggregateReceiveDelta > 0) {
            receiveDelta = aggregateReceiveDelta;
        }
        if (transmitDelta == 0 && aggregateTransmitDelta > 0) {
            transmitDelta = aggregateTransmitDelta;
        }

        nextDownloadSpeed = formatTransferRate(receiveDelta / elapsedSeconds);
        nextUploadSpeed = formatTransferRate(transmitDelta / elapsedSeconds);
    } else {
        m_networkSampleTimer.start();
    }

    m_previousReceiveBytes = receiveBytes;
    m_previousTransmitBytes = transmitBytes;
    m_previousAggregateReceiveBytes = aggregateReceiveBytes;
    m_previousAggregateTransmitBytes = aggregateTransmitBytes;

    if (m_cpuUsage == nextCpuUsage
        && m_memoryUsage == nextMemoryUsage
        && m_downloadSpeedText == nextDownloadSpeed
        && m_uploadSpeedText == nextUploadSpeed) {
        return;
    }

    m_cpuUsage = nextCpuUsage;
    m_memoryUsage = nextMemoryUsage;
    m_downloadSpeedText = nextDownloadSpeed;
    m_uploadSpeedText = nextUploadSpeed;
    emit systemStatsChanged();
}

void FashionLeftPluginProvider::refreshWeather()
{
    ensureWeatherWatchPaths();

    const QString configPath = weatherConfigPath();

    QString nextWeatherCityText;
    QString nextWeatherTemperatureText = QStringLiteral("--°");
    QString nextWeatherSummaryText = QStringLiteral("天气信息不可用");
    QUrl nextWeatherIconSource = QUrl::fromLocalFile(weatherIconPathFor(QString(), true));

    if (QFileInfo::exists(configPath)) {
        QSettings settings(configPath, QSettings::IniFormat);
        nextWeatherCityText = settings.value(QStringLiteral("cache/city")).toString().trimmed();

        bool currentTempOk = false;
        const double currentTemp = settings.value(QStringLiteral("cache/currentTemp")).toDouble(&currentTempOk);
        if (currentTempOk) {
            nextWeatherTemperatureText = QStringLiteral("%1°").arg(qRound(currentTemp));
        }

        bool minTempOk = false;
        bool maxTempOk = false;
        bool weatherCodeOk = false;
        int minTemp = qRound(settings.value(QStringLiteral("cache/todayMin")).toDouble(&minTempOk));
        int maxTemp = qRound(settings.value(QStringLiteral("cache/todayMax")).toDouble(&maxTempOk));
        const int weatherCode = settings.value(QStringLiteral("cache/weatherCode")).toInt(&weatherCodeOk);
        const bool isDay = settings.value(QStringLiteral("cache/isDay"), true).toBool();

        QString descriptionText = weatherCodeOk ? deepinWeatherDescription(weatherCode) : QString();
        QString iconName = weatherCodeOk ? deepinWeatherIconName(weatherCode, isDay) : QString();
        const QByteArray dailyData = settings.value(QStringLiteral("cache/daily")).toByteArray();
        const QJsonDocument dailyDocument = QJsonDocument::fromJson(dailyData);
        if (dailyDocument.isArray() && !dailyDocument.array().isEmpty()) {
            const QJsonObject todayObject = dailyDocument.array().at(0).toObject();
            if (descriptionText.isEmpty()) {
                descriptionText = todayObject.value(QStringLiteral("description")).toString().trimmed();
            }
            if (iconName.isEmpty()) {
                iconName = todayObject.value(QStringLiteral("iconName")).toString().trimmed();
            }

            if (!minTempOk) {
                minTemp = qRound(todayObject.value(QStringLiteral("minTempC")).toDouble());
                minTempOk = todayObject.contains(QStringLiteral("minTempC"));
            }

            if (!maxTempOk) {
                maxTemp = qRound(todayObject.value(QStringLiteral("maxTempC")).toDouble());
                maxTempOk = todayObject.contains(QStringLiteral("maxTempC"));
            }
        }

        if (maxTempOk && minTempOk && maxTemp < minTemp) {
            qSwap(maxTemp, minTemp);
        }

        const QString rangeText = (minTempOk && maxTempOk)
            ? QStringLiteral("%1~%2°C").arg(minTemp).arg(maxTemp)
            : QString();
        if (!descriptionText.isEmpty() && !rangeText.isEmpty()) {
            nextWeatherSummaryText = QStringLiteral("%1 %2").arg(descriptionText, rangeText);
        } else if (!descriptionText.isEmpty()) {
            nextWeatherSummaryText = descriptionText;
        } else if (!rangeText.isEmpty()) {
            nextWeatherSummaryText = rangeText;
        }

        nextWeatherIconSource = QUrl::fromLocalFile(weatherIconPathFor(iconName, isDay));
    }

    if (m_weatherCityText == nextWeatherCityText
        && m_weatherTemperatureText == nextWeatherTemperatureText
        && m_weatherSummaryText == nextWeatherSummaryText
        && m_weatherIconSource == nextWeatherIconSource) {
        return;
    }

    m_weatherCityText = nextWeatherCityText;
    m_weatherTemperatureText = nextWeatherTemperatureText;
    m_weatherSummaryText = nextWeatherSummaryText;
    m_weatherIconSource = nextWeatherIconSource;
    emit weatherChanged();
}

void FashionLeftPluginProvider::onNotificationCountChanged(uint count)
{
    if (m_notificationCount == static_cast<int>(count)) {
        return;
    }

    m_notificationCount = static_cast<int>(count);
    emit notificationCountChanged();
}

bool FashionLeftPluginProvider::launchCommand(const QString &program, const QStringList &arguments)
{
    if (program.isEmpty()) {
        return false;
    }

    return QProcess::startDetached(program, arguments);
}

bool FashionLeftPluginProvider::showControlCenterPage(const QString &pagePath)
{
    QDBusMessage message = QDBusMessage::createMethodCall(ControlCenterService,
                                                         ControlCenterPath,
                                                         ControlCenterInterface,
                                                         QStringLiteral("ShowPage"));
    message << pagePath;
    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    return reply.type() != QDBusMessage::ErrorMessage;
}

QString FashionLeftPluginProvider::commandOutput(const QString &program, const QStringList &arguments, int timeoutMs)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(500);
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QString FashionLeftPluginProvider::executablePathForService(const QString &serviceName)
{
    if (serviceName.isEmpty()) {
        return {};
    }

    QDBusConnectionInterface *connectionInterface = QDBusConnection::sessionBus().interface();
    if (!connectionInterface) {
        return {};
    }

    const QDBusReply<uint> pidReply = connectionInterface->servicePid(serviceName);
    if (!pidReply.isValid() || pidReply.value() == 0) {
        return {};
    }

    const QString executableLinkPath = QStringLiteral("/proc/%1/exe").arg(pidReply.value());
    const QString executablePath = QFileInfo(executableLinkPath).symLinkTarget();
    return QFileInfo(executablePath).exists() ? executablePath : QString();
}

QStringList FashionLeftPluginProvider::desktopSearchDirectories()
{
    QStringList searchDirectories = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    searchDirectories << QStringLiteral("/var/lib/linglong/entries/share/applications")
                      << QStringLiteral("/usr/local/share/applications")
                      << QStringLiteral("/usr/share/applications")
                      << QDir::home().filePath(QStringLiteral(".local/share/applications"));
    return uniqueExistingDirectories(searchDirectories);
}

QString FashionLeftPluginProvider::locateDesktopFile(const QString &desktopId)
{
    if (desktopId.isEmpty()) {
        return {};
    }

    if (QFileInfo::exists(desktopId)) {
        return QFileInfo(desktopId).absoluteFilePath();
    }

    for (const QString &directory : desktopSearchDirectories()) {
        const QString candidate = QDir(directory).filePath(desktopId);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

QString FashionLeftPluginProvider::desktopEntryValue(const QString &desktopFilePath, const QString &key)
{
    if (desktopFilePath.isEmpty()) {
        return {};
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));
    return settings.value(key).toString().trimmed();
}

QString FashionLeftPluginProvider::desktopCommandExecutable(const QString &desktopFilePath)
{
    const QStringList commandLines = {
        desktopEntryValue(desktopFilePath, QStringLiteral("TryExec")),
        desktopEntryValue(desktopFilePath, QStringLiteral("Exec")),
    };

    for (const QString &commandLine : commandLines) {
        const QStringList commandParts = QProcess::splitCommand(commandLine);
        for (const QString &part : commandParts) {
            const QString token = part.trimmed();
            if (token.isEmpty() || token == QStringLiteral("env") || token.startsWith(QLatin1Char('%'))) {
                continue;
            }

            if (token.contains(QLatin1Char('='))
                && !token.startsWith(QLatin1Char('/'))
                && !token.contains(QDir::separator())) {
                continue;
            }

            return token;
        }
    }

    return {};
}

QString FashionLeftPluginProvider::locateDesktopFileByExecutable(const QString &executablePath)
{
    if (executablePath.isEmpty()) {
        return {};
    }

    const QFileInfo executableInfo(executablePath);
    const QString canonicalExecutablePath = executableInfo.canonicalFilePath();
    const QString normalizedExecutablePath = canonicalExecutablePath.isEmpty()
        ? executableInfo.absoluteFilePath()
        : canonicalExecutablePath;
    const QString executableName = QFileInfo(normalizedExecutablePath).fileName();
    if (executableName.isEmpty()) {
        return {};
    }

    for (const QString &directory : desktopSearchDirectories()) {
        QDirIterator iterator(directory,
                              {QStringLiteral("*.desktop")},
                              QDir::Files | QDir::Readable,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString desktopFilePath = iterator.next();
            const QString desktopExecutable = desktopCommandExecutable(desktopFilePath);
            if (desktopExecutable.isEmpty()) {
                continue;
            }

            const QFileInfo desktopExecutableInfo(desktopExecutable);
            const QString canonicalDesktopExecutable = desktopExecutableInfo.canonicalFilePath();
            const QString normalizedDesktopExecutable = canonicalDesktopExecutable.isEmpty()
                ? desktopExecutableInfo.absoluteFilePath()
                : canonicalDesktopExecutable;
            if ((!normalizedDesktopExecutable.isEmpty() && normalizedDesktopExecutable == normalizedExecutablePath)
                || desktopExecutableInfo.fileName() == executableName) {
                return desktopFilePath;
            }
        }
    }

    return {};
}

bool FashionLeftPluginProvider::launchDesktopEntry(const QString &desktopFilePath)
{
    if (desktopFilePath.isEmpty()) {
        return false;
    }

    if (launchCommand(QStringLiteral("gio"), {QStringLiteral("launch"), desktopFilePath})) {
        return true;
    }

    const QString desktopId = QFileInfo(desktopFilePath).completeBaseName();
    if (!desktopId.isEmpty() && launchCommand(QStringLiteral("gtk-launch"), {desktopId})) {
        return true;
    }

    const QString desktopExecutable = desktopCommandExecutable(desktopFilePath);
    if (!desktopExecutable.isEmpty() && launchCommand(desktopExecutable)) {
        return true;
    }

    return false;
}

QString FashionLeftPluginProvider::localizedDesktopEntryValue(const QString &desktopFilePath, const QString &key)
{
    if (desktopFilePath.isEmpty() || key.isEmpty()) {
        return {};
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));

    QStringList localizedKeys;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &uiLanguage : uiLanguages) {
        QString normalizedLanguage = uiLanguage.trimmed();
        if (normalizedLanguage.isEmpty()) {
            continue;
        }

        normalizedLanguage.replace(QLatin1Char('-'), QLatin1Char('_'));
        const QString fullKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage);
        if (!localizedKeys.contains(fullKey)) {
            localizedKeys << fullKey;
        }

        const int separatorIndex = normalizedLanguage.indexOf(QLatin1Char('_'));
        if (separatorIndex > 0) {
            const QString baseLanguageKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage.left(separatorIndex));
            if (!localizedKeys.contains(baseLanguageKey)) {
                localizedKeys << baseLanguageKey;
            }
        }
    }

    for (const QString &localizedKey : localizedKeys) {
        const QString localizedValue = settings.value(localizedKey).toString().trimmed();
        if (!localizedValue.isEmpty()) {
            return localizedValue;
        }
    }

    return settings.value(key).toString().trimmed();
}

QString FashionLeftPluginProvider::musicPlayerIconNameForDesktopEntry(const QString &desktopId,
                                                                      const QString &appName,
                                                                      const QString &serviceName)
{
    QStringList candidateIconNames;
    const auto appendCandidate = [&candidateIconNames](const QString &iconName) {
        const QString trimmedIconName = iconName.trimmed();
        if (!trimmedIconName.isEmpty() && !candidateIconNames.contains(trimmedIconName)) {
            candidateIconNames << trimmedIconName;
        }
    };
    const auto appendMusicAppCandidates = [&appendCandidate](const QString &name) {
        const QString normalizedName = name.trimmed().toLower();
        if (normalizedName.isEmpty()) {
            return;
        }

        if (normalizedName.contains(QStringLiteral("网易云"))
            || normalizedName.contains(QStringLiteral("网易"))
            || normalizedName.contains(QStringLiteral("云音乐"))
            || normalizedName.contains(QStringLiteral("netease"))
            || normalizedName.contains(QStringLiteral("cloudmusic"))) {
            appendCandidate(QStringLiteral("netease-cloud-music"));
            appendCandidate(QStringLiteral("com.netease.cloudmusic"));
        }

        if (normalizedName.contains(QStringLiteral("deepin music"))
            || normalizedName.contains(QStringLiteral("deepin-music"))
            || normalizedName.contains(QStringLiteral("com.deepin.music"))
            || normalizedName.contains(QStringLiteral("deepin音乐"))
            || normalizedName == QStringLiteral("音乐")) {
            appendCandidate(QStringLiteral("deepin-music"));
            appendCandidate(QStringLiteral("deepin-music-player"));
        }
    };

    appendMusicAppCandidates(appName);

    const QString normalizedDesktopId = desktopId.trimmed().toLower();
    if (normalizedDesktopId.contains(QStringLiteral("netease"))
        || desktopId.contains(QStringLiteral("网易云"))) {
        appendCandidate(QStringLiteral("netease-cloud-music"));
        appendCandidate(QStringLiteral("com.netease.cloudmusic"));
    }
    if (normalizedDesktopId.contains(QStringLiteral("deepin-music"))
        || normalizedDesktopId.contains(QStringLiteral("com.deepin.music"))
        || desktopId.contains(QStringLiteral("deepin-music"))
        || desktopId.contains(QStringLiteral("Deepin Music"))) {
        appendCandidate(QStringLiteral("deepin-music"));
        appendCandidate(QStringLiteral("deepin-music-player"));
    }

    const QString desktopFilePath = locateDesktopFile(desktopId);
    const QString iconName = desktopEntryValue(desktopFilePath, QStringLiteral("Icon"));
    appendCandidate(iconName);

    QString fallbackIconName = desktopId;
    if (fallbackIconName.endsWith(QStringLiteral(".desktop"))) {
        fallbackIconName.chop(QStringLiteral(".desktop").size());
    }
    appendCandidate(fallbackIconName);

    for (const QString &candidateIconName : std::as_const(candidateIconNames)) {
        if (!iconPathForName(candidateIconName).isEmpty()) {
            return candidateIconName;
        }
    }

    if (!candidateIconNames.isEmpty()) {
        return candidateIconNames.constFirst();
    }

    if (isBrowserDesktopId(desktopId) || isBrowserServiceName(serviceName) || desktopId.isEmpty()) {
        return DefaultMusicIconName;
    }

    return fallbackIconName.isEmpty() ? DefaultMusicIconName : fallbackIconName;
}

QUrl FashionLeftPluginProvider::iconSourceForName(const QString &iconName)
{
    const QString iconPath = iconPathForName(iconName);
    if (!iconPath.isEmpty()) {
        return QUrl::fromLocalFile(iconPath);
    }

    return QUrl::fromLocalFile(iconPathForName(DefaultMusicIconName));
}

QStringList FashionLeftPluginProvider::mailAccountIdsFromJson(const QString &jsonText)
{
    const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8());
    if (!document.isObject()) {
        return {};
    }

    QStringList accountIds;
    const QJsonArray accounts = document.object().value(QStringLiteral("accounts")).toArray();
    for (const QJsonValue &accountValue : accounts) {
        const QString accountId = accountValue.toObject().value(QStringLiteral("id")).toString().trimmed();
        if (!accountId.isEmpty()) {
            accountIds << accountId;
        }
    }

    return accountIds;
}

int FashionLeftPluginProvider::unreadCountFromJson(const QString &jsonText, bool *ok)
{
    const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8());
    if (!document.isObject()) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }

    const QJsonValue countValue = document.object().value(QStringLiteral("count"));
    if (!countValue.isDouble()) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }

    if (ok) {
        *ok = true;
    }
    return countValue.toInt();
}

bool FashionLeftPluginProvider::readCpuTimes(quint64 *totalTime, quint64 *idleTime)
{
    if (!totalTime || !idleTime) {
        return false;
    }

    QFile file(QStringLiteral("/proc/stat"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString line = QString::fromUtf8(file.readLine()).simplified();
    const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 5 || fields.first() != QStringLiteral("cpu")) {
        return false;
    }

    quint64 total = 0;
    for (qsizetype index = 1; index < fields.size(); ++index) {
        total += fields.at(index).toULongLong();
    }

    const quint64 idle = fields.value(4).toULongLong() + fields.value(5).toULongLong();
    *totalTime = total;
    *idleTime = idle;
    return true;
}

int FashionLeftPluginProvider::systemMemoryUsagePercent()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    quint64 totalMemory = 0;
    quint64 availableMemory = 0;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if (line.startsWith(QStringLiteral("MemTotal:"))) {
            totalMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        } else if (line.startsWith(QStringLiteral("MemAvailable:"))) {
            availableMemory = line.section(QLatin1Char(':'), 1).simplified().section(QLatin1Char(' '), 0, 0).toULongLong();
        }

        if (totalMemory > 0 && availableMemory > 0) {
            break;
        }
    }

    if (totalMemory == 0) {
        return 0;
    }

    const quint64 usedMemory = totalMemory > availableMemory ? (totalMemory - availableMemory) : 0;
    return qBound(0, qRound(static_cast<double>(usedMemory) * 100.0 / totalMemory), 100);
}

QStringList FashionLeftPluginProvider::preferredNetworkInterfaces()
{
    const QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    QHash<QString, QNetworkInterface> interfacesByName;
    QStringList physicalInterfaceNames;
    QStringList runningInterfaceNames;

    for (const QNetworkInterface &networkInterface : allInterfaces) {
        const QString interfaceName = networkInterface.name().trimmed();
        if (!interfaceName.isEmpty()) {
            interfacesByName.insert(interfaceName, networkInterface);
        }

        if (!isUpAndRunningInterface(networkInterface)) {
            continue;
        }

        if (!runningInterfaceNames.contains(interfaceName)) {
            runningInterfaceNames << interfaceName;
        }

        if (isLikelyPhysicalTrafficInterface(networkInterface)
            && !physicalInterfaceNames.contains(interfaceName)) {
            physicalInterfaceNames << interfaceName;
        }
    }

    QStringList defaultPhysicalInterfaceNames;
    QStringList defaultRunningInterfaceNames;
    const QStringList routeInterfaceNames = defaultRouteInterfaceNames();
    for (const QString &interfaceName : routeInterfaceNames) {
        const auto interfaceIterator = interfacesByName.constFind(interfaceName);
        if (interfaceIterator == interfacesByName.cend()) {
            continue;
        }

        const QNetworkInterface networkInterface = interfaceIterator.value();
        if (!isUpAndRunningInterface(networkInterface)) {
            continue;
        }

        if (!defaultRunningInterfaceNames.contains(interfaceName)) {
            defaultRunningInterfaceNames << interfaceName;
        }

        if (isLikelyPhysicalTrafficInterface(networkInterface)
            && !defaultPhysicalInterfaceNames.contains(interfaceName)) {
            defaultPhysicalInterfaceNames << interfaceName;
        }
    }

    if (!defaultPhysicalInterfaceNames.isEmpty()) {
        return defaultPhysicalInterfaceNames;
    }

    if (!defaultRunningInterfaceNames.isEmpty()) {
        return defaultRunningInterfaceNames;
    }

    if (!physicalInterfaceNames.isEmpty()) {
        return physicalInterfaceNames;
    }

    if (!runningInterfaceNames.isEmpty()) {
        return runningInterfaceNames;
    }

    return routeInterfaceNames;
}

void FashionLeftPluginProvider::refreshMailClient()
{
    const QString nextMailDesktopId = commandOutput(QStringLiteral("xdg-mime"),
                                                    {QStringLiteral("query"),
                                                     QStringLiteral("default"),
                                                     QStringLiteral("x-scheme-handler/mailto")});
    const QString nextMailDesktopFilePath = locateDesktopFile(nextMailDesktopId);
    QString nextMailIconName = desktopEntryValue(nextMailDesktopFilePath, QStringLiteral("Icon"));
    QString nextMailClientName = localizedDesktopEntryValue(nextMailDesktopFilePath, QStringLiteral("Name"));
    if (nextMailIconName.isEmpty()) {
        nextMailIconName = DefaultMailIconName;
    }
    if (nextMailClientName.isEmpty()) {
        nextMailClientName = QStringLiteral("邮箱");
    }

    if (m_mailDesktopId == nextMailDesktopId
        && m_mailDesktopFilePath == nextMailDesktopFilePath
        && m_mailIconName == nextMailIconName
        && m_mailClientName == nextMailClientName) {
        return;
    }

    m_mailDesktopId = nextMailDesktopId;
    m_mailDesktopFilePath = nextMailDesktopFilePath;
    m_mailIconName = nextMailIconName;
    m_mailClientName = nextMailClientName;
    emit mailClientChanged();
}

QString FashionLeftPluginProvider::weatherConfigPath()
{
    return QDir::home().filePath(QStringLiteral(".config/deepin/org.deepin.weather.conf"));
}

QString FashionLeftPluginProvider::weatherIconPathFor(const QString &iconName, bool isDay)
{
    const QString mappedPath = weatherAssetPath(weatherAssetNameFor(iconName, isDay));
    if (!mappedPath.isEmpty()) {
        return mappedPath;
    }

    return WeatherAppIconPath;
}

QString FashionLeftPluginProvider::formatTransferRate(double bytesPerSecond)
{
    const double kilobytes = 1024.0;
    const double megabytes = kilobytes * 1024.0;

    if (bytesPerSecond >= megabytes) {
        return QString::number(bytesPerSecond / megabytes, 'f', 1) + QStringLiteral("mb/s");
    }

    if (bytesPerSecond >= kilobytes) {
        return QString::number(bytesPerSecond / kilobytes, 'f', 1) + QStringLiteral("kb/s");
    }

    return QString::number(qRound(bytesPerSecond)) + QStringLiteral("b/s");
}

quint64 FashionLeftPluginProvider::totalInterfaceBytes(bool receiveBytes, const QStringList &preferredInterfaces)
{
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    const QList<QByteArray> lines = file.readAll().split('\n');
    quint64 totalBytes = 0;
    for (const QByteArray &rawLine : lines) {
        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (!line.contains(QLatin1Char(':'))) {
            continue;
        }

        const QStringList nameAndValues = line.split(QLatin1Char(':'), Qt::KeepEmptyParts);
        if (nameAndValues.size() != 2) {
            continue;
        }

        const QString interfaceName = nameAndValues.at(0).trimmed();
        if (interfaceName == QStringLiteral("lo")) {
            continue;
        }

        if (!preferredInterfaces.isEmpty() && !preferredInterfaces.contains(interfaceName)) {
            continue;
        }

        const QStringList values = nameAndValues.at(1).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (values.size() < 16) {
            continue;
        }

        totalBytes += receiveBytes ? values.at(0).toULongLong() : values.at(8).toULongLong();
    }

    return totalBytes;
}

void FashionLeftPluginProvider::ensureWeatherWatchPaths()
{
    if (!m_weatherWatcher) {
        return;
    }

    const QString configPath = weatherConfigPath();
    const QString configDirectory = QFileInfo(configPath).absolutePath();

    if (!m_weatherWatcher->directories().contains(configDirectory)) {
        m_weatherWatcher->addPath(configDirectory);
    }

    if (QFileInfo::exists(configPath) && !m_weatherWatcher->files().contains(configPath)) {
        m_weatherWatcher->addPath(configPath);
    }
}

} // namespace dock
