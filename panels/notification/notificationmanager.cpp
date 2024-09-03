// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationmanager.h"
#include "persistence.h"
#include "notificationsetting.h"
#include "bubblemodel.h"
#include "notificationentity.h"

#include <DDesktopServices>
#include <DSGApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

DCORE_USE_NAMESPACE

namespace notification {

static const QString NotificationsDBusService = "org.freedesktop.Notifications";
static const QString NotificationsDBusPath = "/org/freedesktop/Notifications";
static const QString DDENotifyDBusServer = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusPath = "/org/deepin/dde/Notification1";
    static const QString SessionDBusService = "org.deepin.dde.SessionManager1";
    static const QString SessionDaemonDBusPath = "/org/deepin/dde/SessionManager1";
static const QStringList IgnoreList= {
        "dde-control-center"
};

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_persistence(new Persistence())
    , m_notificationSetting(new NotificationSetting(this))
    , m_bubbles(new BubbleModel(this))
    , m_userSessionManager(new UserSessionManager(SessionDBusService, SessionDaemonDBusPath, QDBusConnection::sessionBus(), this))
{

}

NotificationManager::~NotificationManager()
{
    delete m_persistence;
}

void NotificationManager::invokeDefaultAction(int bubbleIndex)
{
    auto entity = m_bubbles->bubbleItem(bubbleIndex);
    if (!entity)
        return;

    invokeAction(bubbleIndex, entity->defaultActionId());
}

void NotificationManager::invokeAction(int bubbleIndex, const QString &actionId)
{
    auto entity = m_bubbles->bubbleItem(bubbleIndex);
    if (!entity)
        return;

    m_bubbles->remove(bubbleIndex);
    m_persistence->removeOne(entity->storageId());

    Q_EMIT ActionInvoked(entity->replacesId() == 0 ? entity->id() : entity->replacesId(), actionId);
    Q_EMIT NotificationClosed(entity->id(), NotificationManager::Closed);
}

void NotificationManager::close(int bubbleIndex)
{
    auto entity = m_bubbles->bubbleItem(bubbleIndex);
    if (!entity)
        return;

    m_bubbles->remove(bubbleIndex);
    m_persistence->removeOne(entity->storageId());

    Q_EMIT NotificationClosed(entity->id(), NotificationManager::Dismissed);
}

void NotificationManager::delayProcess(int bubbleIndex)
{
    auto entity = m_bubbles->bubbleItem(bubbleIndex);
    if (!entity)
        return;

    m_bubbles->remove(bubbleIndex);

    Q_EMIT NotificationClosed(entity->id(), NotificationManager::Dismissed);
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
    if (calledFromDBus() && m_notificationSetting->getSystemSetting(NotificationSetting::CloseNotification).toBool()) {
        return 0;
    }

    QString appId;
    if (calledFromDBus()) {
        QDBusReply<uint> reply = connection().interface()->servicePid(message().service());
        appId = DSGApplication::getId(reply.value());
    }

    if (appId.isEmpty())
        appId = appName;

    bool enableAppNotification = m_notificationSetting->getAppSetting(appId, NotificationSetting::EnableNotification).toBool();
    if (!enableAppNotification && !IgnoreList.contains(appId)) {
        return 0;
    }

    QString strBody = body;
    strBody.replace(QLatin1String("\\\\"), QLatin1String("\\"), Qt::CaseInsensitive);

    QString strIcon = appIcon;
    if (strIcon.isEmpty())
        strIcon = m_notificationSetting->getAppSetting(appId, NotificationSetting::AppIcon).toString();

    auto entity = new NotificationEntity(appName, replacesId, strIcon, summary, strBody, actions, hints, expireTimeout);
    initConnections(entity);

    bool enablePreview = true, lockScreenShow = true;
    bool dndMode = isDoNotDisturb();
    bool systemNotification = IgnoreList.contains(appName);
    bool lockScreen = m_userSessionManager->locked();

    if (!systemNotification) {
        enablePreview = m_notificationSetting->getAppSetting(appId, NotificationSetting::EnablePreview).toBool();
        lockScreenShow = m_notificationSetting->getAppSetting(appId, NotificationSetting::LockScreenShowNotification).toBool();
    }

    entity->setEnablePreview(enablePreview);

    tryPlayNotificationSound(appName, dndMode, actions, hints);

    if (m_bubbles->isReplaceBubble(entity)) {
        auto oldBubble = m_bubbles->replaceBubble(entity);
        m_persistence->removeOne(oldBubble->storageId());
        oldBubble->deleteLater();
        tryRecordOne(appId, entity);
    } else {
        setReplaceId(entity);

        if (systemNotification) {
            tryRecordOne(appId, entity);
            m_bubbles->push(entity);
        } else if (lockScreen && !lockScreenShow) {
            tryRecordOne(appId, entity);
        } else {
            if (!dndMode && enableAppNotification) {
                m_bubbles->push(entity);
            }
            tryRecordOne(appId, entity);
        }
    }

    // If replaces_id is 0, the return value is a UINT32 that represent the notification.
    // If replaces_id is not 0, the returned value is the same value as replaces_id.
    return replacesId == 0 ? entity->id() : replacesId;
}
void NotificationManager::CloseNotification(uint id)
{
    auto entity = m_bubbles->removeById(id);
    if (entity) {
        m_persistence->removeOne(entity->storageId());
    }
}

QString NotificationManager::GetServerInformation(QString &name, QString &vendor, QString &specVersion)
{
    name = QString("DeepinNotifications");
    vendor = QString("Deepin");
    specVersion = QString("3.0");

    return QString("1.2");
}

QStringList NotificationManager::GetAppList()
{
    return QStringList();
}

QDBusVariant NotificationManager::GetAppInfo(const QString &appId, uint configItem)
{
    return QDBusVariant();
}

void NotificationManager::SetAppInfo(const QString &appId, uint configItem, const QDBusVariant &value)
{

}

QString NotificationManager::GetAppSetting(const QString &appName)
{
    return QString();
}

void NotificationManager::SetAppSetting(const QString &settings)
{

}

void NotificationManager::SetSystemInfo(uint configItem, const QDBusVariant &value)
{

}

QDBusVariant NotificationManager::GetSystemInfo(uint configItem)
{
    return QDBusVariant();
}

void NotificationManager::Toggle()
{

}

void NotificationManager::Show()
{

}

void NotificationManager::Hide()
{

}

void NotificationManager::initConnections(NotificationEntity *entity)
{
    connect(entity, &NotificationEntity::notificationTimeout, this, &NotificationManager::bubbleExpired);
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

bool NotificationManager::isDoNotDisturb() const
{
    if (!m_notificationSetting->getSystemSetting(NotificationSetting::DNDMode).toBool())
        return false;

    // 未点击按钮  任何时候都勿扰模式
    if (!m_notificationSetting->getSystemSetting(NotificationSetting::OpenByTimeInterval).toBool() &&
        !m_notificationSetting->getSystemSetting(NotificationSetting::LockScreenOpenDNDMode).toBool()) {
        return true;
    }

    bool lockScreen = m_userSessionManager->locked();
    // 点击锁屏时 并且 锁屏状态 任何时候都勿扰模式
    if (m_notificationSetting->getSystemSetting(NotificationSetting::LockScreenOpenDNDMode).toBool() && lockScreen)
        return true;

    QTime currentTime = QTime::fromString(QDateTime::currentDateTime().toString("hh:mm"));
    QTime startTime = QTime::fromString(m_notificationSetting->getSystemSetting(NotificationSetting::StartTime).toString());
    QTime endTime = QTime::fromString(m_notificationSetting->getSystemSetting(NotificationSetting::EndTime).toString());

    bool dndMode = false;
    if (startTime < endTime) {
        dndMode = startTime <= currentTime && endTime >= currentTime;
    } else if (startTime > endTime) {
        dndMode = startTime <= currentTime || endTime >= currentTime;
    } else {
        dndMode = true;
    }

    if (dndMode && m_notificationSetting->getSystemSetting(NotificationSetting::OpenByTimeInterval).toBool()) {
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
        playSound = m_notificationSetting->getAppSetting(appName, NotificationSetting::EnableSound).toBool();
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

void NotificationManager::setReplaceId(NotificationEntity *entity)
{
    entity->setId(++m_replaceCount);
    entity->setReplacesId(m_replaceCount);
}

void NotificationManager::tryRecordOne(const QString &appId, NotificationEntity *entity)
{
    bool showInNotificationCenter = m_notificationSetting->getAppSetting(appId, NotificationSetting::ShowInNotificationCenter).toBool();
    if (!showInNotificationCenter) {
        return;
    }

    // "cancel"表示正在发送蓝牙文件,不需要发送到通知中心
    if (entity->body().contains("%") && entity->actions().contains("cancel")) {
        return;
    }

    uint storageId = m_persistence->addOne(entity);
    entity->setStorageId(storageId);

    // TODO send signal
}

void NotificationManager::bubbleExpired(NotificationEntity *entity)
{
    if (!entity)
        return;

    m_bubbles->remove(entity);

    Q_EMIT NotificationClosed(entity->id(), NotificationManager::Expired);
}

} // notification