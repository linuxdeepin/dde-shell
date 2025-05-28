// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationmanager.h"
#include "dataaccessorproxy.h"
#include "dbaccessor.h"
#include "notificationsetting.h"
#include "notifyentity.h"

#include <DDesktopServices>
#include <DSGApplication>
#include <QDBusConnectionInterface>

#include <DConfig>

#include <QAbstractItemModel>
#include <QDBusInterface>
#include <QProcess>
#include <QTimer>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>

#include <appletbridge.h>
#include <pluginloader.h>

DCORE_USE_NAMESPACE
DS_USE_NAMESPACE

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}
namespace notification {

static const uint NoReplacesId = 0;
static const int DefaultTimeOutMSecs = 5000;
static const QString NotificationsDBusService = "org.freedesktop.Notifications";
static const QString NotificationsDBusPath = "/org/freedesktop/Notifications";
static const QString DDENotifyDBusServer = "org.deepin.dde.Notification1";
static const QString DDENotifyDBusPath = "/org/deepin/dde/Notification1";
static const QString SessionDBusService = "org.deepin.dde.SessionManager1";
static const QString SessionDaemonDBusPath = "/org/deepin/dde/SessionManager1";

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_persistence(DataAccessorProxy::instance())
    , m_setting(new NotificationSetting(this))
    , m_pendingTimeout(new QTimer(this))
{
    m_pendingTimeout->setSingleShot(true);
    connect(m_pendingTimeout, &QTimer::timeout, this, &NotificationManager::onHandingPendingEntities);

    DataAccessorProxy::instance()->setSource(DBAccessor::instance());

    DAppletBridge bridge("org.deepin.ds.dde-apps");
    if (auto apps = bridge.applet()) {
        if (auto model = apps->property("appModel").value<QAbstractItemModel *>()) {
            m_setting->setAppAccessor(model);
        }
    }
    if (!m_setting->appAccessor()) {
        qWarning(notifyLog) << "It's not exist appModel for the applet:" << bridge.pluginId();
    }
    connect(m_setting, &NotificationSetting::appAdded, this, &NotificationManager::AppAdded);
    connect(m_setting, &NotificationSetting::appRemoved, this, &NotificationManager::AppRemoved);
    connect(m_setting, &NotificationSetting::appValueChanged, this, [this] (const QString &appId, uint configItem, const QVariant &value) {
        Q_EMIT AppInfoChanged(appId, configItem, QDBusVariant(value));
    });
    connect(m_setting, &NotificationSetting::systemValueChanged, this, [this] (uint configItem, const QVariant &value) {
        Q_EMIT SystemInfoChanged(configItem, QDBusVariant(value));
    });

    QScopedPointer<DConfig> config(DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification"));
    m_systemApps = config->value("systemApps").toStringList();
    // TODO temporary fix for AppNamesMap
    m_appNamesMap = config->value("AppNamesMap").toMap();

    if (QStringLiteral("wayland") != QGuiApplication::platformName()) {
        initScreenLockedState();
    }
}

NotificationManager::~NotificationManager()
{
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
    return m_persistence->fetchEntityCount(DataAccessor::AllApp(), NotifyEntity::Processed);
}

void NotificationManager::actionInvoked(qint64 id, const QString &actionKey)
{
    qInfo(notifyLog) << "Action invoked, id:" << id << ", actionKey" << actionKey;
    auto entity = m_persistence->fetchEntity(id);
    if (entity.isValid()) {
        doActionInvoked(entity, actionKey);

        entity.setProcessedType(NotifyEntity::Removed);
        updateEntityProcessed(entity);
    }
}

void NotificationManager::actionInvoked(qint64 id, uint bubbleId, const QString &actionKey)
{
    qInfo(notifyLog) << "Action invoked, bubbleId:" << bubbleId << ", id:" << id << ", actionKey" << actionKey;
    actionInvoked(id, actionKey);

    Q_EMIT ActionInvoked(bubbleId, actionKey);
    Q_EMIT NotificationClosed(bubbleId, NotifyEntity::Closed);
}

void NotificationManager::notificationClosed(qint64 id, uint bubbleId, uint reason)
{
    qDebug(notifyLog) << "Close notification id" << id << ", reason" << reason;
    updateEntityProcessed(id, reason);

    Q_EMIT NotificationClosed(bubbleId, reason);
}

void NotificationManager::notificationReplaced(qint64 id)
{
    updateEntityProcessed(id, NotifyEntity::Closed);
}

void NotificationManager::removeNotification(qint64 id)
{
    m_persistence->removeEntity(id);

    emitRecordCountChanged();
}

void NotificationManager::removeNotifications(const QString &appName)
{
    m_persistence->removeEntityByApp(appName);

    emitRecordCountChanged();
}

void NotificationManager::removeNotifications()
{
    m_persistence->clear();

    emitRecordCountChanged();
}

QStringList NotificationManager::GetCapabilities()
{
    QStringList result;
    result << "action-icons" << "actions" << "body" << "body-hyperlinks" << "body-markup" << "body-image" << "enable-sound" << "persistence";

    return result;
}

uint NotificationManager::Notify(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                                 const QString &body, const QStringList &actions, const QVariantMap &hints,
                                 int expireTimeout)
{
    qDebug(notifyLog) << "Notify"
            << ", appName:" << appName
            << ", summary:" << summary
            << ", appIcon:" << appIcon
            << ", body size:" << body.size()
            << ", actions:" << actions
            << ", hint: " << hints
            << ", replaceId:" << replacesId
            << ", expireTimeout:" << expireTimeout;

    if (calledFromDBus() && m_setting->systemValue(NotificationSetting::CloseNotification).toBool()) {
        qDebug(notifyLog) << "Notify has been disabled by CloseNotification setting.";
        return 0;
    }

    QString appId = appIdByAppName(appName);

    if (appId.isEmpty() && calledFromDBus()) {
        QDBusReply<uint> reply = connection().interface()->servicePid(message().service());
        appId = DSGApplication::getId(reply.value());
    }

    if (appId.isEmpty())
        appId = appName;

    bool enableAppNotification = m_setting->appValue(appId, NotificationSetting::EnableNotification).toBool();
    if (!enableAppNotification && !m_systemApps.contains(appId)) {
        return 0;
    }

    auto tsAppName = m_setting->appValue(appId, NotificationSetting::AppName).toString();
    if (tsAppName.isEmpty()) {
        tsAppName = appName;
    }

    QString strBody = body;
    strBody.replace(QLatin1String("\\\\"), QLatin1String("\\"), Qt::CaseInsensitive);

    QString strIcon = appIcon;
    if (strIcon.isEmpty())
        strIcon = m_setting->appValue(appId, NotificationSetting::AppIcon).toString();
    NotifyEntity entity(tsAppName, replacesId, strIcon, summary, strBody, actions, hints, expireTimeout);
    entity.setAppId(appId);
    entity.setProcessedType(NotifyEntity::None);
    entity.setReplacesId(replacesId);

    bool lockScreenShow = true;
    bool dndMode = isDoNotDisturb();
    bool systemNotification = m_systemApps.contains(appId);
    const bool desktopScreen = !m_screenLocked;

    if (!systemNotification) {
        lockScreenShow = m_setting->appValue(appId, NotificationSetting::ShowOnLockScreen).toBool();
    }
    const bool onDesktopShow = m_setting->appValue(appId, NotificationSetting::ShowOnDesktop).toBool();

    tryPlayNotificationSound(entity, appId, dndMode);

    // new one
    if (replacesId == NoReplacesId) {
        entity.setBubbleId(++m_replacesCount);

        if (systemNotification) { // 系统通知
            entity.setProcessedType(NotifyEntity::NotProcessed);
        } else if (m_screenLocked && !lockScreenShow) { // 锁屏不显示通知
            entity.setProcessedType(NotifyEntity::Processed);
        } else if (desktopScreen && !onDesktopShow) { // 桌面不显示通知
            entity.setProcessedType(NotifyEntity::Processed);
        } else { // 锁屏显示通知或者未锁屏状态
            if (!systemNotification && !dndMode && enableAppNotification) { // 普通应用非勿扰模式并且开启通知选项
                entity.setProcessedType(NotifyEntity::NotProcessed);
            } else {
                entity.setProcessedType(NotifyEntity::Processed);
            }
        }
    } else { // maybe replace one
        entity.setBubbleId(replacesId);
        entity.setProcessedType(NotifyEntity::NotProcessed);
    }

    if (entity.processedType() != NotifyEntity::None) {
        if (!recordNotification(entity)) {
            return 0;
        }

        if (entity.isReplace() && m_persistence->fetchLastEntity(entity.bubbleId()).isValid()) {
            removePendingEntity(entity);
        }

        emitRecordCountChanged();

        Q_EMIT NotificationStateChanged(entity.id(), entity.processedType());

        bool critical = false;
        if (auto iter = hints.find("urgency"); iter != hints.end()) {
            critical = iter.value().toUInt() == NotifyEntity::Critical;
        }
        // 0: never expire. -1: DefaultTimeOutMSecs
        if (expireTimeout != 0 && !critical) {
            pushPendingEntity(entity, expireTimeout);
        }
    }

    qInfo(notifyLog) << "Notify done, bubbleId:" << entity.bubbleId() << ", id:" << entity.id() << ", type:" << entity.processedType();

    // If replaces_id is 0, the return value is a UINT32 that represent the notification.
    // If replaces_id is not 0, the returned value is the same value as replaces_id.
    return entity.bubbleId();
}
void NotificationManager::CloseNotification(uint id)
{
    auto entity = m_persistence->fetchLastEntity(id);
    if (entity.isValid()) {
        entity.setProcessedType(NotifyEntity::Removed);
        updateEntityProcessed(entity);
    }

    Q_EMIT NotificationClosed(id, NotifyEntity::Closed);
    qDebug(notifyLog) << "Close notify, bubbleId" << id << ", id:" << entity.id();
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
    qDebug(notifyLog) << "Get appList";
    return m_setting->apps();
}

QVariant NotificationManager::GetAppInfo(const QString &appId, uint configItem)
{
    return m_setting->appValue(appId, static_cast<NotificationSetting::AppConfigItem>(configItem));
}

void NotificationManager::SetAppInfo(const QString &appId, uint configItem, const QVariant &value)
{
    qDebug(notifyLog) << "Set appInfo" << appId << configItem << value;
    return m_setting->setAppValue(appId, static_cast<NotificationSetting::AppConfigItem>(configItem), value);
}

void NotificationManager::SetSystemInfo(uint configItem, const QVariant &value)
{
    qDebug(notifyLog) << "Set systemInfo" << configItem << value;
    return m_setting->setSystemValue(static_cast<NotificationSetting::SystemConfigItem>(configItem), value);
}

QVariant NotificationManager::GetSystemInfo(uint configItem)
{
    qDebug(notifyLog) << "Get systemInfo" << configItem;
    return m_setting->systemValue(static_cast<NotificationSetting::SystemConfigItem>(configItem));
}

bool NotificationManager::isDoNotDisturb() const
{
    if (!m_setting->systemValue(NotificationSetting::DNDMode).toBool())
        return false;

    // 未点击按钮  任何时候都勿扰模式
    if (!m_setting->systemValue(NotificationSetting::OpenByTimeInterval).toBool() &&
        !m_setting->systemValue(NotificationSetting::LockScreenOpenDNDMode).toBool()) {
        return true;
    }

    // 点击锁屏时 并且 锁屏状态 任何时候都勿扰模式
    if (m_setting->systemValue(NotificationSetting::LockScreenOpenDNDMode).toBool() && m_screenLocked)
        return true;

    QTime currentTime = QTime::fromString(QDateTime::currentDateTime().toString("hh:mm"));
    QTime startTime = QTime::fromString(m_setting->systemValue(NotificationSetting::StartTime).toString());
    QTime endTime = QTime::fromString(m_setting->systemValue(NotificationSetting::EndTime).toString());

    bool dndMode = true;
    if (startTime < endTime) {
        dndMode = startTime <= currentTime && endTime >= currentTime;
    } else if (startTime > endTime) {
        dndMode = startTime <= currentTime || endTime >= currentTime;
    }

    return dndMode && m_setting->systemValue(NotificationSetting::OpenByTimeInterval).toBool();
}

bool NotificationManager::recordNotification(NotifyEntity &entity)
{
    qint64 id = -1;
    if (entity.isReplace()) {
        auto lastEntity = m_persistence->fetchLastEntity(entity.bubbleId());
        if (lastEntity.isValid()) {
            bool showInNotifyCenter = true;
            if (entity.hints().contains("x-deepin-ShowInNotifyCenter")) {
                showInNotifyCenter = entity.hints()["x-deepin-ShowInNotifyCenter"].toBool();
            }
            if (showInNotifyCenter) {
                m_persistence->updateEntityProcessedType(lastEntity.id(), NotifyEntity::Processed);
            } else {
                id = m_persistence->replaceEntity(lastEntity.id(), entity);
            }
        } else {
            qWarning() << "Not exist notification to replace for the replaceId" << entity.replacesId();
        }
    }
    if (id == -1) {
        id = m_persistence->addEntity(entity);
    }

    if (id == -1) {
        qWarning(notifyLog) << "Failed on saving DB, bubbleId:" << entity.bubbleId() << ", appName" << entity.appName();
        return false;
    }

    entity.setId(id);

    return true;
}

void NotificationManager::tryPlayNotificationSound(const NotifyEntity &entity, const QString &appId, bool dndMode) const
{
    const auto hints = entity.hints();
    if (!hints.isEmpty() && (!hints.value("enable-sound", true).toBool() ||
        !hints.value("x-deepin-PlaySound", true).toBool())) {
        return;
    }

    bool playSoundTip = false;
    bool playSound = true;
    bool systemNotification = m_systemApps.contains(appId);
    if (!systemNotification)
        playSound = m_setting->appValue(appId, NotificationSetting::EnableSound).toBool();

    if (playSound && !dndMode) {
        const auto actions = entity.actions();
        //接收蓝牙文件时，只在发送完成后才有提示音,"cancel"表示正在发送文件
        if (actions.contains("cancel")) {
            if (auto iter = hints.find("x-deepin-action-_view"); iter != hints.end()) {
                const auto action = iter.value().toString();
                if (action.contains("xdg-open"))
                    playSoundTip = true;
            }
        } else {
            playSoundTip = true;
        }
    }

    if (playSoundTip) {
        Dtk::Gui::DDesktopServices::playSystemSoundEffect(Dtk::Gui::DDesktopServices::SSE_Notifications);
    }
}

void NotificationManager::emitRecordCountChanged()
{
    const auto count = m_persistence->fetchEntityCount(DataAccessor::AllApp(), NotifyEntity::Processed);
    emit RecordCountChanged(count);
}

void NotificationManager::pushPendingEntity(const NotifyEntity &entity, int expireTimeout)
{
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

void NotificationManager::updateEntityProcessed(qint64 id, uint reason)
{
    auto entity = m_persistence->fetchEntity(id);
    if (entity.isValid()) {
        if ((reason == NotifyEntity::Closed || reason == NotifyEntity::Dismissed) && entity.processedType() == NotifyEntity::NotProcessed) {
            entity.setProcessedType(NotifyEntity::Removed);
        } else {
            entity.setProcessedType(NotifyEntity::Processed);
        }
        updateEntityProcessed(entity);
    }
}

void NotificationManager::updateEntityProcessed(const NotifyEntity &entity)
{
    const auto id = entity.id();
    const bool removed = entity.processedType() == NotifyEntity::Removed;
    bool showInCenter = m_setting->appValue(entity.appId(), NotificationSetting::ShowInCenter).toBool();
    if (entity.hints().contains("x-deepin-ShowInNotifyCenter")) {
        showInCenter = entity.hints()["x-deepin-ShowInNotifyCenter"].toBool();
    }
    if (removed || !showInCenter) {
        // remove it from memory
        m_persistence->removeEntity(id);
    } else {
        // add to db and remove it form memory
        m_persistence->updateEntityProcessedType(id, entity.processedType());
    }

    Q_EMIT NotificationStateChanged(entity.id(), entity.processedType());

    removePendingEntity(entity);
    emitRecordCountChanged();
}

QString NotificationManager::appIdByAppName(const QString &appName) const
{
    const auto items = m_setting->appItems();
    for (const auto &item: items) {
        if (item.id == appName || item.appName == appName)
            return item.id;
    }

    if (m_appNamesMap.contains(appName)) {
        return m_appNamesMap.value(appName).toString();
    }

    return QString();
}

void NotificationManager::doActionInvoked(const NotifyEntity &entity, const QString &actionId)
{
    qDebug(notifyLog) << "Invoke the notification:" << entity.id() << entity.appName() << actionId;
    QMap<QString, QVariant> hints = entity.hints();
    QMap<QString, QVariant>::const_iterator i = hints.constBegin();
    while (i != hints.constEnd()) {
        if (i.key() == "x-deepin-action-" + actionId) {
            QStringList args;
            if (i.value().typeId() == QMetaType::QStringList) {
                args = i.value().toStringList();
            } else {
                qDebug(notifyLog) << "Deprecate hint format, use string list instead of string."
                                  << "actionId:" << actionId << ", value:" << i.value();
                args = i.value().toString().split(",");
            }
            if (!args.isEmpty()) {
                QString cmd = args.takeFirst(); // 命令

                QProcess pro;
                pro.setProgram(cmd);
                pro.setArguments(args);
                QProcessEnvironment proEnv = QProcessEnvironment::systemEnvironment();
                proEnv.remove("DSG_APP_ID");
                pro.setProcessEnvironment(proEnv);
                pro.startDetached();
            }
        } else if (i.key() == "deepin-dde-shell-action-" + actionId) {
            const QString data(i.value().toString());
            if (!invokeShellAction(data)) {
                qWarning(notifyLog) << "Failed to invoke the action" << actionId;
            }
        }
        ++i;
    }
}

// Helper function to convert QList to variadic parameters.
template<typename... Args, std::size_t... Is>
static bool invokeMethodWithListImpl(QObject *obj, const char *member, Qt::ConnectionType c, const QList<QGenericArgument> &list, std::index_sequence<Is...>)
{
    return QMetaObject::invokeMethod(obj, member, c, list[Is]...);
}

static bool invokeMethodWithList(QObject *obj, const char *member, Qt::ConnectionType c, const QVariantList &list)
{
    const int MaxArgumentCount = 9;
    QList<QGenericArgument> args;
    args.resize(MaxArgumentCount);
    for (int i = 0; i < list.size(); i++) {
        const auto &item = list.at(i);
        args[i] = QGenericArgument{item.typeName(), item.data()};
    }
    return invokeMethodWithListImpl(obj, member, c, args, std::make_index_sequence<MaxArgumentCount>());
}

bool NotificationManager::invokeShellAction(const QString &data)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(data.toLocal8Bit(), &error);
    if (error.error != QJsonParseError::NoError || doc.isNull() || !doc.isObject()) {
        qWarning(notifyLog) << "Failed to parse data" << error.errorString();
        return false;
    }
    const QString PluginId("pluginId");
    const QString Method("method");
    QJsonObject root(doc.object());
    if (!root.contains(PluginId) || !root.contains(Method)) {
        qWarning(notifyLog) << "Incorrect format, missing 'pluginId' or 'method' keyword";
        return false;
    }
    const auto pluginId = root[PluginId].toString();
    DAppletBridge bridge(pluginId);
    if (!bridge.isValid()) {
        qWarning(notifyLog) << "Doesn't exit the applet" << pluginId;
        return false;
    }

    auto meta = DS_NAMESPACE::DPluginLoader::instance()->plugin(pluginId);
    const auto visiblity = meta.value("Visibility");
    if (visiblity != "Public") {
        qWarning(notifyLog) << "Can't access the applet" << pluginId;
        return false;
    }

    const auto method = root[Method].toString();
    const QVariantList arguments = root["arguments"].toArray().toVariantList();
    if (auto applet = bridge.applet()) {
        return invokeMethodWithList(applet, method.toStdString().c_str(), Qt::DirectConnection, arguments);
    }
    return false;
}

void NotificationManager::initScreenLockedState()
{
    const QString interfaceAndServiceName = "org.deepin.dde.LockFront1";
    const QString path = "/org/deepin/dde/LockFront1";

    QDBusInterface interface(interfaceAndServiceName, path,
        "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());

    QDBusReply<QDBusVariant> reply = interface.call("Get", "org.deepin.dde.LockFront1", "Visible");
    if (reply.isValid()) {
        m_screenLocked = reply.value().variant().toBool();
    } else {
        m_screenLocked = false;
        qWarning(notifyLog) << "Failed to get the lock visible property:" << reply.error().message();
    }

    QDBusConnection::sessionBus().connect(interfaceAndServiceName, path, interfaceAndServiceName,
        "Visible", this, SLOT(onScreenLockedChanged(bool)));
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
        // let timer start in main thread
        QMetaObject::invokeMethod(m_pendingTimeout, "start", Qt::QueuedConnection, Q_ARG(int, newInterval));
    } else {
        // reset m_lastTimeoutPoint
        m_lastTimeoutPoint = std::numeric_limits<qint64>::max();
    }

    for (const auto &item : timeoutEntities) {
        qDebug(notifyLog) << "Expired for the notification " << item.id() << item.appName();
        notificationClosed(item.id(), item.bubbleId(), NotifyEntity::Expired);
    }
}

void NotificationManager::removePendingEntity(const NotifyEntity &entity)
{
    for (auto iter = m_pendingTimeoutEntities.begin(); iter != m_pendingTimeoutEntities.end();) {
        const auto item = iter.value();
        if (item == entity || (entity.isReplace() && item.bubbleId() == entity.bubbleId())) {
            m_pendingTimeoutEntities.erase(iter);
            onHandingPendingEntities();
            break;
        }
        ++iter;
    }
}

void NotificationManager::onScreenLockedChanged(bool screenLocked)
{
    m_screenLocked = screenLocked;
}

} // notification
