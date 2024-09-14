// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyaccessor.h"

#include <QQmlEngine>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QProcess>
#include <QElapsedTimer>
#include <QDBusReply>

#include "notifyentity.h"

namespace notifycenter {
namespace {
Q_LOGGING_CATEGORY(notifyLog, "notify")
}

static const QString DDENotifyDBusServer = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusInterface = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusPath = "/org/deepin/dde/Notification1";
static const uint ShowNotificationTop = 7;

static QDBusInterface notifyCenterInterface()
{
    return QDBusInterface(DDENotifyDBusServer, DDENotifyDBusPath, DDENotifyDBusInterface);
}

static QDBusInterface controlCenterInterface()
{
    return QDBusInterface("org.deepin.dde.ControlCenter1",
                          "/org/deepin/dde/ControlCenter1",
                          "org.deepin.dde.ControlCenter1");
}

class Benchmark
{
public:
    explicit Benchmark(const QString &msg)
        : m_msg(msg)
    {
        m_timer.start();
    }
    ~Benchmark()
    {
        const auto time = m_timer.elapsed();
        if (time > 10)
            qWarning(notifyLog) << m_msg << " cost more time, elapsed:" << time;
    }
private:
    QElapsedTimer m_timer;
    QString m_msg;
};
#define BENCHMARK() \
    Benchmark __benchmark__(__FUNCTION__);

class EventFilter : public QObject
{
    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut
            || event->type() == QEvent::KeyPress  || event->type() == QEvent::KeyRelease) {
            qDebug(notifyLog) << "Focus and Key event" << event->type() << watched << qApp->focusObject();
        }
        return false;
    }
};

NotifyAccessor::NotifyAccessor(QObject *parent)
{
    auto connection = QDBusConnection::sessionBus();
    bool valid = connection.connect(DDENotifyDBusServer, DDENotifyDBusPath, DDENotifyDBusInterface,
                                    "RecordAdded", this, SLOT(onReceivedRecord(const QString &)));
    if (!valid) {
        qWarning() << "NotifyConnection is invalid, and can't receive RecordAdded signal.";
    }

    if (!qEnvironmentVariableIsEmpty("DS_NOTIFICATION_DEBUG")) {
        const int value = qEnvironmentVariableIntValue("DS_NOTIFICATION_DEBUG");
        m_debuging = value;
    }
    if (m_debuging) {
        qApp->installEventFilter(new EventFilter());
    }
}

NotifyAccessor *NotifyAccessor::instance()
{
    static NotifyAccessor *instance = nullptr;

    if (!instance) {
        instance = new NotifyAccessor(qGuiApp);
        instance->setDataAccessor(new DataAccessor());
    }
    return instance;
}

NotifyAccessor *NotifyAccessor::create(QQmlEngine *, QJSEngine *)
{
    auto helper = NotifyAccessor::instance();
    QQmlEngine::setObjectOwnership(helper, QQmlEngine::CppOwnership);
    return helper;
}

void NotifyAccessor::setDataAccessor(DataAccessor *accessor)
{
    if (m_accessor) {
        delete m_accessor;
    }
    m_accessor = accessor;
}

NotifyEntity NotifyAccessor::fetchEntity(const QString &id) const
{
    qDebug(notifyLog) << "Fetch entity" << id;
    BENCHMARK();
    auto ret = m_accessor->fetchEntity(id);
    return ret;
}

int NotifyAccessor::fetchEntityCount(const QString &appName) const
{
    qDebug(notifyLog) << "Fetch entity count for the app" << appName;
    BENCHMARK();
    auto ret = m_accessor->fetchEntityCount(appName);
    return ret;
}

NotifyEntity NotifyAccessor::fetchLastEntity(const QString &appName) const
{
    qDebug(notifyLog) << "Fetch last entity for the app" << appName;
    BENCHMARK();
    auto ret = m_accessor->fetchLastEntity(appName);
    return ret;
}

QList<NotifyEntity> NotifyAccessor::fetchEntities(const QString &appName, int maxCount)
{
    qDebug(notifyLog) << "Fetch entities for the app" << appName;
    auto ret = m_accessor->fetchEntities(appName, maxCount);
    return ret;
}

QStringList NotifyAccessor::fetchApps(int maxCount) const
{
    qDebug(notifyLog) << "Fetch apps count" << maxCount;
    BENCHMARK();
    auto ret = m_accessor->fetchApps(maxCount);
    return ret;
}

void NotifyAccessor::removeEntity(const QString &id)
{
    qDebug(notifyLog) << "Remove notify" << id;
    BENCHMARK();

    // TODO it maybe cause 'database is locked' by remove DB directly.
    QDBusReply<void> reply = notifyCenterInterface().call("RemoveRecord",
                                                          id);
    if (reply.error().isValid()) {
        qWarning(notifyLog) << "Failed to remove the notification" << id << reply.error().message();
        return;
    }

    // m_accessor->removeEntity(id);

    appsChanged();
    dataInfoChanged();
}

void NotifyAccessor::removeEntityByApp(const QString &appName)
{
    qDebug(notifyLog) << "Remove notifies for the application" << appName;
    BENCHMARK();

    // TODO server doesn't provide a way to remove notification by appName.
    const auto notifies = fetchEntities(appName);
    for (auto item : notifies) {
        notifyCenterInterface().call("RemoveRecord", item.id());
    }

    // m_accessor->removeEntityByApp(appName);

    appsChanged();
    dataInfoChanged();
}

void NotifyAccessor::clear()
{
    qDebug(notifyLog) << "Remove all notify";

    // TODO it maybe cause 'database is locked' by remove DB directly.
    QDBusReply<void> reply = notifyCenterInterface().call("ClearRecords");
    if (reply.error().isValid()) {
        qWarning(notifyLog) << "Failed to remove all notification" << reply.error().message();
        return;
    }

    // m_accessor->clear();

    appsChanged();
    dataInfoChanged();
}

// don't need to emit ActionInvoked of protocol.
void NotifyAccessor::invokeAction(const NotifyEntity &entity, const QString &actionId)
{
    qDebug(notifyLog) << "Invoke action for the notify" << entity.id() << actionId;

    QMap<QString, QVariant> hints = NotifyEntity::parseHint(entity.hint());
    if (hints.isEmpty())
        return;
    QMap<QString, QVariant>::const_iterator i = hints.constBegin();
    while (i != hints.constEnd()) {
        QStringList args = i.value().toString().split(",");
        if (!args.isEmpty()) {
            QString cmd = args.first();
            args.removeFirst();
            if (i.key() == "x-deepin-action-" + actionId) {
                qDebug(notifyLog) << "Invoke action" << cmd;
                QProcess::startDetached(cmd, args);
            }
        }
        ++i;
    }
}

void NotifyAccessor::pinApplication(const QString &appName, bool pin)
{
    qDebug(notifyLog) << "Pin the application" << appName << pin;
    QDBusReply<void> reply = notifyCenterInterface().call("SetAppInfo",
                                 appName,
                                 ShowNotificationTop,
                                 QVariant::fromValue(QDBusVariant(pin)));
    if (reply.error().isValid()) {
        qWarning(notifyLog) << "Failed to set Pin of the application" << appName << pin << reply.error().message();
        return;
    }
    m_pinnedApps[appName] = pin;
}

bool NotifyAccessor::applicationPin(const QString &appName) const
{
    if (auto iter = m_pinnedApps.find(appName); iter != m_pinnedApps.end())
        return iter.value();

    QDBusReply<QVariant> reply = notifyCenterInterface().call("GetAppInfo",
                                 appName,
                                 ShowNotificationTop);
    if (reply.error().isValid()) {
        qWarning(notifyLog) << "Failed to get Pin of the application" << appName << reply.error().message();
        return false;
    }
    const auto data = reply.value().toBool();
    const_cast<NotifyAccessor *>(this)->m_pinnedApps[appName] = data;
    return data;
}

void NotifyAccessor::openNotificationSetting()
{
    qDebug(notifyLog) << "Open notification setting";
    QDBusReply<void> reply = controlCenterInterface().call("ShowPage",
                                                           "notification");
    if (reply.error().isValid()) {
        qWarning(notifyLog) << "Failed to Open notifycation setting" << reply.error().message();
        return;
    }
}

void NotifyAccessor::addNotify(const QString &appName, const QString &content)
{
    qDebug(notifyLog) << "Add notify" << appName;
    m_accessor->addNotify(appName, content);

    if (auto entity = fetchLastEntity(appName); entity.isValid()) {
        entityReceived(entity.id());
    }
    tryEmitAppsChanged(appName);
    dataInfoChanged();
}

void NotifyAccessor::onReceivedRecord(const QString &id)
{
    dataInfoChanged();
    emit entityReceived(id);
}

void NotifyAccessor::tryEmitAppsChanged(const QString &appName)
{
    const auto apps = fetchApps();
    if (!apps.contains(appName)) {
        appsChanged();
    }
}

QString NotifyAccessor::dataInfo() const
{
    QStringList info;
    auto entityCount = fetchEntityCount();
    auto apps = fetchApps();
    info.append(QString("notifyCount: %1, appCount: %2").arg(entityCount).arg(apps.size()));
    for (auto item : apps) {
        info.append(QString("%1 -> %2").arg(item).arg(fetchEntityCount(item)));
    }
    QString ret = info.join("\n");
    return ret;
}

QStringList NotifyAccessor::apps() const
{
    return m_apps;
}

bool NotifyAccessor::debuging() const
{
    return m_debuging;
}

}
