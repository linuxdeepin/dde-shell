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
#include <QDBusReply>

#include <DConfig>

#include "dataaccessor.h"

DCORE_USE_NAMESPACE

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}
namespace notifycenter {

static const uint ShowNotificationTop = 7;
static const QString InvalidApp {"DS-Invalid-Apps"};
static const QStringList InvalidPinnedApps {InvalidApp};

static QDBusInterface controlCenterInterface()
{
    return QDBusInterface("org.deepin.dde.ControlCenter1",
                          "/org/deepin/dde/ControlCenter1",
                          "org.deepin.dde.ControlCenter1");
}

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
    : m_pinnedApps(InvalidPinnedApps)
{
    Q_UNUSED(parent)
    if (!qEnvironmentVariableIsEmpty("DS_NOTIFICATION_DEBUG")) {
        const int value = qEnvironmentVariableIntValue("DS_NOTIFICATION_DEBUG");
        m_debugging = value;
    }
    if (m_debugging) {
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

void NotifyAccessor::setDataUpdater(QObject *updater)
{
    m_dataUpdater = updater;
}

bool NotifyAccessor::enabled() const
{
    return m_enabled;
}

void NotifyAccessor::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

NotifyEntity NotifyAccessor::fetchEntity(qint64 id) const
{
    qDebug(notifyLog) << "Fetch entity" << id;
    auto ret = m_accessor->fetchEntity(id);
    return ret;
}

int NotifyAccessor::fetchEntityCount(const QString &appName) const
{
    qDebug(notifyLog) << "Fetch entity count for the app" << appName;
    auto ret = m_accessor->fetchEntityCount(appName, NotifyEntity::Processed);
    return ret;
}

NotifyEntity NotifyAccessor::fetchLastEntity(const QString &appName) const
{
    qDebug(notifyLog) << "Fetch last entity for the app" << appName;
    auto ret = m_accessor->fetchLastEntity(appName, NotifyEntity::Processed);
    return ret;
}

QList<NotifyEntity> NotifyAccessor::fetchEntities(const QString &appName, int maxCount)
{
    qDebug(notifyLog) << "Fetch entities for the app" << appName;
    auto ret = m_accessor->fetchEntities(appName, NotifyEntity::Processed, maxCount);
    return ret;
}

QStringList NotifyAccessor::fetchApps(int maxCount) const
{
    qDebug(notifyLog) << "Fetch apps count" << maxCount;
    auto ret = m_accessor->fetchApps(maxCount);
    return ret;
}

void NotifyAccessor::removeEntity(qint64 id)
{
    qDebug(notifyLog) << "Remove notify" << id;
    QMetaObject::invokeMethod(m_dataUpdater, "removeNotification", Qt::DirectConnection, Q_ARG(qint64, id));
}

void NotifyAccessor::removeEntityByApp(const QString &appName)
{
    qDebug(notifyLog) << "Remove notifies for the application" << appName;

    QMetaObject::invokeMethod(m_dataUpdater, "removeNotifications", Qt::DirectConnection, Q_ARG(const QString &, appName));
}

void NotifyAccessor::clear()
{
    qDebug(notifyLog) << "Remove all notify";

    QMetaObject::invokeMethod(m_dataUpdater, "removeNotifications", Qt::DirectConnection);
}

void NotifyAccessor::closeNotify(const NotifyEntity &entity, NotifyEntity::ClosedReason reason)
{
    if (!m_dataUpdater)
        return;
    const auto id = entity.id();
    const auto bubbleId = entity.bubbleId();
    QMetaObject::invokeMethod(m_dataUpdater, "notificationClosed", Qt::DirectConnection, Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, reason));
}

void NotifyAccessor::invokeNotify(const NotifyEntity &entity, const QString &actionId)
{
    if (!m_dataUpdater)
        return;
    const auto id = entity.id();
    const auto bubbleId = entity.bubbleId();
    qDebug(notifyLog) << "Invoke notify" << id << actionId;
    QMetaObject::invokeMethod(m_dataUpdater, "actionInvoked", Qt::DirectConnection,
                              Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(const QString&, actionId));

}

// don't need to emit ActionInvoked of protocol.
void NotifyAccessor::invokeAction(const NotifyEntity &entity, const QString &actionId)
{
    qDebug(notifyLog) << "Invoke action for the notify" << entity.id() << actionId;

    if (!m_dataUpdater)
        return;
    const auto id = entity.id();
    QMetaObject::invokeMethod(m_dataUpdater, "actionInvoked", Qt::DirectConnection, Q_ARG(qint64, id), Q_ARG(const QString &, actionId));
}

void NotifyAccessor::pinApplication(const QString &appId, bool pin)
{
    qDebug(notifyLog) << "Pin the application" << appId << pin;

    if (!pin) {
        m_pinnedApps.removeOne(appId);
    } else {
        if (!m_pinnedApps.contains(appId))
            m_pinnedApps.append(appId);
    }
    QScopedPointer<DConfig> config(DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification"));
    config->setValue("pinnedApps", m_pinnedApps);
}

bool NotifyAccessor::applicationPin(const QString &appId) const
{
    if (m_pinnedApps.contains(appId))
        return true;

    if (m_pinnedApps.contains(InvalidApp)) {
        QScopedPointer<DConfig> config(DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification"));
        const_cast<NotifyAccessor*>(this)->m_pinnedApps = config->value("pinnedApps").toStringList();
    }

    return m_pinnedApps.contains(appId);
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

void NotifyAccessor::onNotificationStateChanged(qint64 id, int processedType)
{
    if (!enabled())
        return;
    if (processedType == NotifyEntity::Processed) {
        emit entityReceived(id);
        emit stagingEntityClosed(id);
    } else if (processedType == NotifyEntity::NotProcessed) {
        emit stagingEntityReceived(id);
    }
}

void NotifyAccessor::onReceivedRecord(const QString &id)
{
    emit entityReceived(id.toLongLong());
}

bool NotifyAccessor::debugging() const
{
    return m_debugging;
}

}
