// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationmanager.h"
#include "notificationsetting.h"
#include "notifyentity.h"
#include "dbaccessor.h"

#include <applet.h>
#include <containment.h>
#include <pluginloader.h>

#include <QDateTime>
#include <DDesktopServices>
#include <DSGApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

DCORE_USE_NAMESPACE
DS_USE_NAMESPACE

namespace notification {

Q_LOGGING_CATEGORY(notifyServerLog, "dde.shell.notification.server")

static const uint NoReplacesId = 0;
const int DefaultTimeOutMSecs = 5000;
static const QString NotificationsDBusService = "org.freedesktop.Notifications";
static const QString NotificationsDBusPath = "/org/freedesktop/Notifications";
static const QString DDENotifyDBusServer = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusPath = "/org/deepin/dde/Notification1";
static const QString SessionDBusService = "org.deepin.dde.SessionManager1";
static const QString SessionDaemonDBusPath = "/org/deepin/dde/SessionManager1";
static const QStringList IgnoreList= {
        "dde-control-center"
};

static QList<DApplet *> appletList(const QString &pluginId)
{
    QList<DApplet *> ret;
    auto rootApplet = DPluginLoader::instance()->rootApplet();
    auto root = qobject_cast<DContainment *>(rootApplet);

    QQueue<DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (applet->pluginId() == pluginId)
                ret << applet;
        }
    }
    return ret;
}

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_persistence(DBAccessor::instance())
    , m_notificationSetting(new NotificationSetting(this))
    , m_userSessionManager(new UserSessionManager(SessionDBusService, SessionDaemonDBusPath, QDBusConnection::sessionBus(), this))
    , m_pendingTimeout(new QTimer(this))
{
    m_pendingTimeout->setSingleShot(true);
    connect(m_pendingTimeout, &QTimer::timeout, this, &NotificationManager::onHandingPendingEntities);

    auto applets = appletList("org.deepin.ds.dde-apps");
    if (!applets.isEmpty()) {
        if (auto apps = applets.first()) {
            if (auto model = apps->property("appModel").value<QAbstractListModel*>()) {
                m_notificationSetting->setAppAccessor(model);
            }
        }
    }
    connect(m_notificationSetting, &NotificationSetting::appAdded, this, &NotificationManager::AppAdded);
    connect(m_notificationSetting, &NotificationSetting::appRemoved, this, &NotificationManager::AppRemoved);
    connect(m_notificationSetting, &NotificationSetting::appValueChanged, this, [this] (const QString &appId, uint configItem, const QVariant &value) {
        Q_EMIT AppInfoChanged(appId, configItem, QDBusVariant(value));
    });
    connect(m_notificationSetting, &NotificationSetting::systemValueChanged, this, [this] (uint configItem, const QVariant &value) {
        Q_EMIT SystemInfoChanged(configItem, QDBusVariant(value));
    });
}

NotificationManager::~NotificationManager()
{
    delete m_persistence;
}

bool NotificationManager::registerDbusService()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.interface()->registerService(NotificationsDBusService,
                                            QDBusConnectionInterface::ReplaceExistingService,
                                            QDBusConnectionInterface::AllowReplacement);
    if (!connection.registerObject(NotificationsDBusPath, this)) {
        return false;
    }

    QDBusConnection ddeNotifyConnect = QDBusConnection::sessionBus();
    ddeNotifyConnect.interface()->registerService(DDENotifyDBusServer,
                                                  QDBusConnectionInterface::ReplaceExistingService,
                                                  QDBusConnectionInterface::AllowReplacement);
    if (!ddeNotifyConnect.registerObject(DDENotifyDBusPath, this)) {
        return false;
    }

    return true;
}

uint NotificationManager::recordCount() const
{
    return m_persistence->fetchEntityCount(QLatin1String(), NotifyEntity::processedValue());
}

void NotificationManager::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    m_persistence->updateEntityProcessedType(id, NotifyEntity::processedValue());
    Q_EMIT notificationStateChanged(id, NotifyEntity::processedValue());

    Q_EMIT ActionInvoked(bubbleId, actionKey);
    Q_EMIT NotificationClosed(bubbleId, NotifyEntity::Closed);

    emitRecordCountChanged();
}

void NotificationManager::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    m_persistence->updateEntityProcessedType(id, NotifyEntity::processedValue());
    Q_EMIT notificationStateChanged(id, NotifyEntity::processedValue());

    Q_EMIT NotificationClosed(bubbleId, reason);

    emitRecordCountChanged();
}

void NotificationManager::notificationReplaced(qint64 id)
{
    m_persistence->removeEntity(id);
    Q_EMIT notificationStateChanged(id, NotifyEntity::removedValue());

    emitRecordCountChanged();
}

QStringList NotificationManager::GetCapabilities()
{
    QStringList result;
    result << "action-icons" << "actions" << "body" << "body-hyperlinks" << "body-markup" << "body-images" << "persistence";

    return result;
}

uint NotificationManager::Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                                 const QString &body, const QStringList &actions, const QVariantMap &hints,
                                 int expireTimeout)
{
    qInfo() << "Notify" << appName << replacesId << appIcon << summary << body << actions << hints << expireTimeout;
    if (calledFromDBus() && m_notificationSetting->systemValue(NotificationSetting::CloseNotification).toBool()) {
        return 0;
    }

    QString appId;
    if (calledFromDBus()) {
        QDBusReply<uint> reply = connection().interface()->servicePid(message().service());
        appId = DSGApplication::getId(reply.value());
    }

    if (appId.isEmpty())
        appId = appName;

    bool enableAppNotification = m_notificationSetting->appValue(appId, NotificationSetting::EnableNotification).toBool();
    if (!enableAppNotification && !IgnoreList.contains(appId)) {
        return 0;
    }

    QString strBody = body;
    strBody.replace(QLatin1String("\\\\"), QLatin1String("\\"), Qt::CaseInsensitive);

    QString strIcon = appIcon;
    if (strIcon.isEmpty())
        strIcon = m_notificationSetting->appValue(appId, NotificationSetting::AppIcon).toString();
    NotifyEntity entity(appName, replacesId, strIcon, summary, strBody, actions, hints, expireTimeout);
    entity.setProcessedType(NotifyEntity::NotProcessed);

    bool enablePreview = true, lockScreenShow = true;
    bool dndMode = isDoNotDisturb();
    bool systemNotification = IgnoreList.contains(appName);
    bool lockScreen = m_userSessionManager->locked();

    if (!systemNotification) {
        enablePreview = m_notificationSetting->appValue(appId, NotificationSetting::EnablePreview).toBool();
        lockScreenShow = m_notificationSetting->appValue(appId, NotificationSetting::LockScreenShowNotification).toBool();
    }

    entity.setEnablePreview(enablePreview);

    tryPlayNotificationSound(appName, dndMode, actions, hints);

    // new one
    if (replacesId == NoReplacesId) {
        entity.setBubbleId(++m_replacesCount);
        tryRecordEntity(appId, entity);

        if (systemNotification || (!dndMode && enableAppNotification && (lockScreenShow || !lockScreen))) {
            Q_EMIT notificationStateChanged(entity.id(), entity.processedType());
        }
    } else { // maybe replace one
        entity.setBubbleId(replacesId);
        tryRecordEntity(appId, entity);

        Q_EMIT notificationStateChanged(entity.id(), entity.processedType());
    }

    bool critical = false;
    if (auto iter = hints.find("urgency"); iter != hints.end()) {
        critical = iter.value().toUInt() == NotifyEntity::Critical;
    }
    // 0: never expire. -1: DefaultTimeOutMSecs
    if (expireTimeout != 0 && !critical) {
        pushPendingEntity(entity);
    }

    // If replaces_id is 0, the return value is a UINT32 that represent the notification.
    // If replaces_id is not 0, the returned value is the same value as replaces_id.
    return replacesId == 0 ? entity.bubbleId() : replacesId;
}
void NotificationManager::CloseNotification(uint id)
{
    // TODO If the notification no longer exists, an empty D-BUS Error message is sent back.
    const auto entity = m_persistence->fetchLastEntity(id);
    if (entity.isValid()) {
        Q_EMIT notificationStateChanged(entity.id(), entity.processedType());
    }

    Q_EMIT NotificationClosed(id, NotifyEntity::Closed);
}

void NotificationManager::GetServerInformation(QString &name, QString &vendor, QString &version, QString &specVersion)
{
    name = QString("DeepinNotifications");
    vendor = QString("Deepin");
    version = QString("3.0");

    specVersion = QString("1.2");
}

QStringList NotificationManager::GetAppList()
{
    qDebug(notifyServerLog) << "Get appList";
    return m_notificationSetting->apps();
}

QDBusVariant NotificationManager::GetAppInfo(const QString &appId, uint configItem)
{
    const auto info = m_notificationSetting->appValue(appId, static_cast<NotificationSetting::AppConfigItem>(configItem));
    return QDBusVariant(info);
}

void NotificationManager::SetAppInfo(const QString &appId, uint configItem, const QDBusVariant &value)
{
    qDebug(notifyServerLog) << "Set appInfo" << appId << configItem << value.variant();
    return m_notificationSetting->setAppValue(appId, static_cast<NotificationSetting::AppConfigItem>(configItem), value.variant());
}

void NotificationManager::SetSystemInfo(uint configItem, const QDBusVariant &value)
{
    qDebug(notifyServerLog) << "Set systemInfo" << configItem << value.variant();
    return m_notificationSetting->setSystemValue(static_cast<NotificationSetting::SystemConfigItem>(configItem), value.variant());
}

QDBusVariant NotificationManager::GetSystemInfo(uint configItem)
{
    qDebug(notifyServerLog) << "Get systemInfo" << configItem;
    const auto value = m_notificationSetting->systemValue(static_cast<NotificationSetting::SystemConfigItem>(configItem));
    return QDBusVariant(value);
}

bool NotificationManager::isDoNotDisturb() const
{
    if (!m_notificationSetting->systemValue(NotificationSetting::DNDMode).toBool())
        return false;

    // 未点击按钮  任何时候都勿扰模式
    if (!m_notificationSetting->systemValue(NotificationSetting::OpenByTimeInterval).toBool() &&
        !m_notificationSetting->systemValue(NotificationSetting::LockScreenOpenDNDMode).toBool()) {
        return true;
    }

    bool lockScreen = m_userSessionManager->locked();
    // 点击锁屏时 并且 锁屏状态 任何时候都勿扰模式
    if (m_notificationSetting->systemValue(NotificationSetting::LockScreenOpenDNDMode).toBool() && lockScreen)
        return true;

    QTime currentTime = QTime::fromString(QDateTime::currentDateTime().toString("hh:mm"));
    QTime startTime = QTime::fromString(m_notificationSetting->systemValue(NotificationSetting::StartTime).toString());
    QTime endTime = QTime::fromString(m_notificationSetting->systemValue(NotificationSetting::EndTime).toString());

    bool dndMode = false;
    if (startTime < endTime) {
        dndMode = startTime <= currentTime && endTime >= currentTime;
    } else if (startTime > endTime) {
        dndMode = startTime <= currentTime || endTime >= currentTime;
    } else {
        dndMode = true;
    }

    if (dndMode && m_notificationSetting->systemValue(NotificationSetting::OpenByTimeInterval).toBool()) {
        return dndMode;
    } else {
        return false;
    }
}

void NotificationManager::tryPlayNotificationSound(const QString &appName, bool dndMode, const QStringList &actions,
                                                   const QVariantMap &hints) const
{
    auto playSoundTip = [] {
        Dtk::Gui::DDesktopServices::playSystemSoundEffect(Dtk::Gui::DDesktopServices::SSE_Notifications);
    };

    bool playSound = true;
    bool systemNotification = IgnoreList.contains(appName);
    if (!systemNotification) {
        playSound = m_notificationSetting->appValue(appName, NotificationSetting::EnableSound).toBool();
    }

    if (playSound && !dndMode) {
        QString action;
        //接收蓝牙文件时，只在发送完成后才有提示音,"cancel"表示正在发送文件
        if (actions.contains("cancel") && hints.contains("x-deepin-action-_view")) {
            action = hints["x-deepin-action-_view"].toString();
            if (action.contains("xdg-open")) {
                playSoundTip();
                return;
            }
        } else {
            playSoundTip();
        }
    }

    if (systemNotification && dndMode) {
        playSoundTip();
    }
}

void NotificationManager::tryRecordEntity(const QString &appId, NotifyEntity &entity)
{
    bool showInNotificationCenter = m_notificationSetting->appValue(appId, NotificationSetting::ShowInNotificationCenter).toBool();
    if (!showInNotificationCenter) {
        return;
    }

    // "cancel"表示正在发送蓝牙文件,不需要发送到通知中心
    if (entity.body().contains("%") && entity.actions().contains("cancel")) {
        return;
    }

    qint64 id = m_persistence->addEntity(entity);
    entity.setId(id);

    emitRecordCountChanged();
}

void NotificationManager::emitRecordCountChanged()
{
    const auto count = m_persistence->fetchEntityCount(QLatin1String(), NotifyEntity::processedValue());
    emit RecordCountChanged(count);
}

void NotificationManager::pushPendingEntity(const NotifyEntity &entity)
{
    const int expireTimeout = entity.expiredTimeout();
    const int interval = expireTimeout == -1 ? DefaultTimeOutMSecs : expireTimeout;

    qint64 point = QDateTime::currentMSecsSinceEpoch() + interval;
    m_pendingTimeoutEntities.insert(point, entity);

    if (m_lastTimeoutPoint > point) {
        m_lastTimeoutPoint = point;
        auto newInterval = m_lastTimeoutPoint - QDateTime::currentMSecsSinceEpoch();
        m_pendingTimeout->setInterval(newInterval);
        m_pendingTimeout->start();
    }
}

void NotificationManager::onHandingPendingEntities()
{
    QList<NotifyEntity> timeoutEntities;

    const auto current = QDateTime::currentMSecsSinceEpoch();
    for (auto iter = m_pendingTimeoutEntities.begin(); iter != m_pendingTimeoutEntities.end();) {
        const auto point = iter.key();
        if (point > current) {
            iter++;
            continue;
        }

        const auto entity = iter.value();;
        timeoutEntities << entity;
        iter = m_pendingTimeoutEntities.erase(iter);
    }

    // update pendingTimeout to deal with m_pendingTimeoutEntities
    if (!m_pendingTimeoutEntities.isEmpty()) {
        auto points = m_pendingTimeoutEntities.keys();
        std::sort(points.begin(), points.end());
        // find last point to restart pendingTimeout
        m_lastTimeoutPoint = points.first();
        auto newInterval = m_lastTimeoutPoint - current;
        m_pendingTimeout->setInterval(newInterval);
        m_pendingTimeout->start();
    } else {
        // reset m_lastTimeoutPoint
        m_lastTimeoutPoint = std::numeric_limits<qint64>::max();
    }

    for (const auto item : timeoutEntities) {
        qDebug(notifyServerLog) << "Expired for the notification " << item.id() << item.appName();
        notificationClosed(item.id(), item.bubbleId(), NotifyEntity::Expired);
    }
}

} // notification
