// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "launchdurationreporter.h"
#include "applicationmanager1interface.h"

#ifdef HAVE_DDE_API_EVENTLOGGER
#include <dde-api/eventlogger.hpp>
#endif

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent>

#include <sys/syscall.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(launchDurationReporter, "org.deepin.dde.shell.dock.launchDurationReporter")

namespace {

constexpr auto kAmService = "org.desktopspec.ApplicationManager1";
constexpr auto kAmPath = "/org/desktopspec/ApplicationManager1";
constexpr auto kInstanceIface = "org.desktopspec.ApplicationManager1.Instance";
constexpr auto kPackageCacheTTLSeconds = 1800;

// pidfd_open is available since Linux 5.3; glibc may not wrap it, so call the syscall directly.
int pidfd_open(pid_t pid, unsigned int flags)
{
    return static_cast<int>(syscall(SYS_pidfd_open, pid, flags));
}

struct InstanceIdentity {
    QString instanceId;
    QString launchType;
};

// Map the just-appeared window (by pid) to its exact ApplicationManager instance via Identify(pidfd),
// reading that instance's LaunchType from the same reply. This is reliable per-window, unlike
// enumerating Application.Instances and guessing the latest one.
InstanceIdentity identifyInstance(pid_t pid)
{
    InstanceIdentity identity;
    if (pid <= 0) {
        return identity;
    }

    const int pidfd = pidfd_open(pid, 0);
    if (pidfd < 0) {
        qCWarning(launchDurationReporter) << "[DockIconTiming] pidfd_open failed for pid:" << pid;
        return identity;
    }

    ApplicationManager am(QString::fromUtf8(kAmService),
                          QString::fromUtf8(kAmPath),
                          QDBusConnection::sessionBus());
    am.setTimeout(1000);

    QDBusObjectPath instancePath;
    ObjectInterfaceMap instanceInfo;
    const QDBusReply<QString> reply = am.Identify(QDBusUnixFileDescriptor(pidfd), instancePath, instanceInfo);
    close(pidfd);

    if (!reply.isValid()) {
        return identity;
    }

    identity.instanceId = instancePath.path().section(QLatin1Char('/'), -1);
    identity.launchType = instanceInfo.value(QString::fromUtf8(kInstanceIface))
                              .value(QStringLiteral("LaunchType")).toString().trimmed();
    if (identity.launchType.isEmpty()) {
        identity.launchType = QStringLiteral("unknown");
    }

    return identity;
}

QString resolveDesktopFilePath(const QString &desktopId, const QString &desktopSourcePath)
{
    QString desktopFilePath = desktopSourcePath;
    if (desktopFilePath.isEmpty() || !QFileInfo::exists(desktopFilePath)) {
        const auto desktopFileName = desktopId.endsWith(QStringLiteral(".desktop")) ? desktopId : desktopId + QStringLiteral(".desktop");
        desktopFilePath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, desktopFileName);
    }

    return desktopFilePath;
}

QString linglongIdFromDesktopFile(const QString &desktopFilePath)
{
    if (desktopFilePath.isEmpty()) {
        return QString();
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    return settings.value(QStringLiteral("Desktop Entry/X-linglong")).toString().trimmed();
}

QString queryLinglongVersion(const QString &linglongId)
{
    QProcess proc;
    proc.start(QStringLiteral("ll-cli"), {QStringLiteral("--json"), QStringLiteral("info"), linglongId});
    if (!proc.waitForFinished(1000)) {
        qCWarning(launchDurationReporter) << "[DockIconTiming] ll-cli info timeout for" << linglongId;
        return QString();
    }

    if (proc.exitCode() != 0) {
        return QString();
    }

    const auto document = QJsonDocument::fromJson(proc.readAllStandardOutput());
    if (!document.isObject()) {
        return QString();
    }

    return document.object().value(QStringLiteral("version")).toString().trimmed();
}

QString queryDebVersion(const QString &desktopFilePath)
{
    if (desktopFilePath.isEmpty()) {
        return QString();
    }

    // The desktopId is often a reverse-DNS id (e.g. org.deepin.dde.control-center) that is NOT the
    // deb package name, so reverse-lookup the owning package from the .desktop file path.
    //
    // dpkg's data dir defaults to /var/lib/dpkg (overridable via DPKG_ADMINDIR); per-package file
    // lists live under <admindir>/info/*.list. grepping those directly is several times faster than
    // `dpkg -S`, which parses its whole database. When that dir is missing (non-standard layout) we
    // fall back to `dpkg -S` so correctness never depends on the directory guess.
    const auto infoDir = qEnvironmentVariable("DPKG_ADMINDIR", QStringLiteral("/var/lib/dpkg")) + QStringLiteral("/info");

    QString packageName;
    if (QFileInfo::exists(infoDir)) {
        QProcess search;
        search.start(QStringLiteral("grep"),
                     {QStringLiteral("-rlFx"), QStringLiteral("--include=*.list"), desktopFilePath, infoDir});
        if (!search.waitForFinished(1000)) {
            qCWarning(launchDurationReporter) << "[DockIconTiming] grep dpkg file list timeout for" << desktopFilePath;
            return QString();
        }
        // grep exit code: 0 = matched, 1 = no match, >1 = error; empty output means no owning package.
        const auto listPath = QString::fromUtf8(search.readAllStandardOutput()).section(QLatin1Char('\n'), 0, 0).trimmed();
        if (!listPath.isEmpty()) {
            // <admindir>/info/<package>[:arch].list -> <package>
            packageName = QFileInfo(listPath).completeBaseName().section(QLatin1Char(':'), 0, 0);
        }
    } else {
        QProcess search;
        search.start(QStringLiteral("dpkg"), {QStringLiteral("-S"), desktopFilePath});
        if (!search.waitForFinished(2000)) {
            qCWarning(launchDurationReporter) << "[DockIconTiming] dpkg -S timeout for" << desktopFilePath;
            return QString();
        }
        if (search.exitCode() == 0) {
            // Output format: "package[:arch][, package2 ...]: /path/to/file".
            packageName = QString::fromUtf8(search.readAllStandardOutput())
                              .section(QLatin1Char(':'), 0, 0).section(QLatin1Char(','), 0, 0).trimmed();
        }
    }

    if (packageName.isEmpty()) {
        return QString();
    }

    QProcess query;
    query.start(QStringLiteral("dpkg-query"), {QStringLiteral("-W"), QStringLiteral("-f=${Version}"), packageName});
    if (!query.waitForFinished(1000)) {
        qCWarning(launchDurationReporter) << "[DockIconTiming] dpkg-query timeout for" << packageName;
        return QString();
    }
    if (query.exitCode() != 0) {
        return QString();
    }

    return QString::fromUtf8(query.readAllStandardOutput()).trimmed();
}

}

namespace dock {

LaunchDurationReporter::LaunchDurationReporter(QObject *parent)
    : QObject(parent)
{
}

LaunchDurationReporter::~LaunchDurationReporter()
{
    m_workerPool.waitForDone();
}

void LaunchDurationReporter::reportWindowAppeared(const QString &desktopId, const QString &desktopSourcePath, pid_t pid)
{
    if (desktopId.isEmpty()) {
        return;
    }

    auto future = QtConcurrent::run(&m_workerPool, [this, desktopId, desktopSourcePath, pid]() {
        const auto identity = identifyInstance(pid);
        const QString uniqueId = identity.instanceId;
        const QString launchType = identity.launchType;

        if (uniqueId.isEmpty()) {
            return;
        }

        const auto desktopFilePath = resolveDesktopFilePath(desktopId, desktopSourcePath);
        const auto linglongId = linglongIdFromDesktopFile(desktopFilePath);
        const auto packageName = linglongId.isEmpty() ? desktopId : linglongId;
        QString version;
        QString pakType;

        {
            QMutexLocker locker(&m_cacheMutex);
            const auto entry = m_packageCache.value(packageName);
            if ((QDateTime::currentSecsSinceEpoch() - entry.timestamp) <= kPackageCacheTTLSeconds) {
                version = entry.version;
                pakType = entry.pakType;
            }
        }

        if (pakType.isEmpty()) {
            if (!linglongId.isEmpty()) {
                version = queryLinglongVersion(linglongId);
                pakType = QStringLiteral("linglong");
            } else {
                version = queryDebVersion(desktopFilePath);
                pakType = version.isEmpty() ? QStringLiteral("unknown") : QStringLiteral("deb");
            }

            QMutexLocker locker(&m_cacheMutex);
            m_packageCache.insert(packageName, {version, pakType, QDateTime::currentSecsSinceEpoch()});
        }

        QMetaObject::invokeMethod(this, [this, desktopId, uniqueId, launchType, version, pakType]() {
            doReport(desktopId, uniqueId, launchType, version, pakType);
        }, Qt::QueuedConnection);
    });
    Q_UNUSED(future)
}

void LaunchDurationReporter::doReport(const QString &desktopId,
                                      const QString &uniqueId,
                                      const QString &launchType,
                                      const QString &version,
                                      const QString &pakType)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610003,
        desktopId,
        QJsonObject{
            {"app_name", desktopId},
            {"launch_type", launchType},
            {"app_version", version},
            {"unique_id", uniqueId},
            {"time", QDateTime::currentMSecsSinceEpoch()},
            {"app_package_type", pakType},
        },
    });
#else
    Q_UNUSED(desktopId)
    Q_UNUSED(uniqueId)
    Q_UNUSED(launchType)
    Q_UNUSED(version)
    Q_UNUSED(pakType)
#endif
}

}
