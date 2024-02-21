// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globals.h"
#include "abstractwindow.h"
#include "desktopfileamparser.h"
#include "desktopfileabstractparser.h"

#include <unistd.h>
#include <sys/syscall.h>

#include <DDBusSender>
#include <QDBusConnection>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(amdesktopfileLog, "dde.shell.dock.amdesktopfile")

// AM static string
static const QString AM_DBUS_PATH = "org.desktopspec.ApplicationManager1";
static const QString DESKTOP_ENTRY_ICON_KEY = "Desktop Entry";
static const QString DEFAULT_KEY = "default";

static int pidfd_open(pid_t pid, uint flags)
{
    // WARNING pidfd_open only support on linux-5.3 or later
    return syscall(SYS_pidfd_open, pid, flags);
}

DS_BEGIN_NAMESPACE
namespace dock {
static QDBusServiceWatcher dbusWatcher(AM_DBUS_PATH, QDBusConnection::sessionBus(),
                                                QDBusServiceWatcher::WatchForOwnerChange);

DesktopFileAMParser::DesktopFileAMParser(QString id, QObject* parent)
    : DesktopfileAbstractParser(id, parent)
{
    auto ifc = QDBusConnection::sessionBus().interface();
    m_amIsAvaliable = ifc->isServiceRegistered(AM_DBUS_PATH);

    connect(&dbusWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](){
        m_amIsAvaliable = true;
        Q_EMIT iconChanged();
    });

    connect(&dbusWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](){
        m_amIsAvaliable = false;
        Q_EMIT iconChanged();
    });

    qCDebug(amdesktopfileLog()) << "create a am desktopfile object: " << m_id;
    m_applicationInterface.reset(new Application(AM_DBUS_PATH, id2dbusPath(id), QDBusConnection::sessionBus(), this));
}

DesktopFileAMParser::~DesktopFileAMParser()
{
    qCDebug(amdesktopfileLog()) << "destroy a am desktopfile object: " << m_id;
}

QString DesktopFileAMParser::id()
{
    if (!m_amIsAvaliable) return DesktopfileAbstractParser::id();

    if (m_id.isEmpty() && m_applicationInterface) {
        m_id = m_applicationInterface->iD();
    }
    return m_id;
}

QString DesktopFileAMParser::name()
{
    if (!m_amIsAvaliable) return DesktopfileAbstractParser::name();
    if (m_name.isEmpty() && m_applicationInterface) {
        updateLocalName();
    }
    return m_name;
}

QString DesktopFileAMParser::desktopIcon()
{
    if (!m_amIsAvaliable) return DesktopfileAbstractParser::desktopIcon();

    if(m_icon.isEmpty() && m_applicationInterface) {
        updateDesktopIcon();
    }

    return m_icon;
}

QString DesktopFileAMParser::genericName()
{
    if (!m_amIsAvaliable) return DesktopfileAbstractParser::genericName();

    if(m_genericName.isEmpty() && m_applicationInterface) {
        updateLocalGenericName();
    }

    return m_genericName;
}

std::pair<bool, QString> DesktopFileAMParser::isValied()
{
    return std::make_pair(true, "has am as backend");
}

QList<QPair<QString, QString>> DesktopFileAMParser::actions()
{
    if (!m_amIsAvaliable) return DesktopfileAbstractParser::actions();

    if(m_actions.isEmpty() && m_applicationInterface) {
        updateActions();
    }
    return m_actions;
}

QString DesktopFileAMParser::id2dbusPath(const QString& id)
{
    return QStringLiteral("/org/desktopspec/ApplicationManager1/") + escapeToObjectPath(id);
}

QString DesktopFileAMParser::identifyWindow(QPointer<AbstractWindow> window)
{
    if (!m_amIsAvaliable) return QString();

    auto pidfd = pidfd_open(window->pid(),0);
    auto res = DDBusSender().service("org.desktopspec.ApplicationManager1")
                                         .interface("org.desktopspec.ApplicationManager1")
                                         .path("/org/desktopspec/ApplicationManager1")
                                         .method("Identify")
                                         .arg(QDBusUnixFileDescriptor(pidfd))
                                         .call();
    res.waitForFinished();
    close(pidfd);
    if (res.isValid()) {
        auto reply = res.reply();
        QList<QVariant> data = reply.arguments();
        return data.first().toString();
    }

    qCDebug(amdesktopfileLog()) << "AM failed to identify, reason is: " << res.error().message();

    return QString();
}

QString DesktopFileAMParser::identifyType()
{
    return QStringLiteral("amAPP");
}

QString DesktopFileAMParser::type()
{
    return identifyType();
}

void DesktopFileAMParser::launch()
{
    m_applicationInterface->Launch(QString(), QStringList(), QVariantMap());
}

void DesktopFileAMParser::launchWithAction(const QString& action)
{
    m_applicationInterface->Launch(action, QStringList(), QVariantMap());
}

void DesktopFileAMParser::requestQuit()
{

}

void DesktopFileAMParser::connectToAmDBusSignal(const QString& signalName, const char *slot)
{
    QDBusConnection::sessionBus().connect(
        m_applicationInterface->service(),
        m_applicationInterface->path(),
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "sa{sv}as",
        this,
        SLOT(onPropertyChanged(const QDBusMessage &))
    );
}

void DesktopFileAMParser::updateActions()
{
    m_actions.clear();

    QString currentLanguageCode = QLocale::system().name();
    QList<QPair<QString, QString>> array;
    auto actions = m_applicationInterface->actions();
    auto actionNames = m_applicationInterface->actionName();

    for (auto action : actions) {
        auto localeName = actionNames.value(action).value(currentLanguageCode);
        auto fallbackDefaultName = actionNames.value(action).value(DEFAULT_KEY);
        m_actions.append({action, localeName.isEmpty() ? fallbackDefaultName : localeName});
    }
}

void DesktopFileAMParser::updateLocalName()
{
    QString currentLanguageCode = QLocale::system().name();
    auto names = m_applicationInterface->name();
    auto localeName = names.value(currentLanguageCode);
    auto fallbackName = names.value(DEFAULT_KEY);
    m_name = localeName.isEmpty() ? fallbackName : localeName;
}

void DesktopFileAMParser::updateDesktopIcon()
{
    m_icon = m_applicationInterface->icons().value(DESKTOP_ENTRY_ICON_KEY);
}

void DesktopFileAMParser::updateLocalGenericName()
{
    QString currentLanguageCode = QLocale::system().name();
    auto genericNames = m_applicationInterface->genericName();
    auto localeGenericName = genericNames.value(currentLanguageCode);
    auto fallBackGenericName = genericNames.value(DEFAULT_KEY);
    m_genericName = localeGenericName.isEmpty() ? fallBackGenericName : localeGenericName;
}

void DesktopFileAMParser::onPropertyChanged(const QDBusMessage &msg)
{
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count())
        return;

    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName != QStringLiteral("org.desktopspec.ApplicationManager1.Application"))
        return;

    QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
    if (changedProps.contains("Name")) {
        updateLocalName();
        Q_EMIT nameChanged();
    } else if (changedProps.contains("Actions")) {
        updateActions();
        Q_EMIT actionsChanged();
    } else if (changedProps.contains("GenericName")) {
        updateLocalGenericName();
        Q_EMIT genericNameChanged();
    } else if (changedProps.contains("Name")) {
        updateLocalName();
        Q_EMIT nameChanged();
    }
}
}
DS_END_NAMESPACE
