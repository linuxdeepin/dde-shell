// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationsetting.h"

#include <QVariant>
#include <QAbstractListModel>

#include <DConfig>

namespace notification {

static const QString InvalidApp {"DS-Invalid-Apps"};
namespace {
enum Roles {
    DesktopIdRole = Qt::UserRole + 1,
    NameRole,
    IconNameRole,
    StartUpWMClassRole,
    NoDisplayRole,
    ActionsRole,
    DDECategoryRole,
    InstalledTimeRole,
    LastLaunchedTimeRole,
    LaunchedTimesRole,
    DockedRole,
    OnDesktopRole,
    AutoStartRole,
    ModelExtendedRole = 0x1000
};
}

NotificationSetting::NotificationSetting(QObject *parent)
    : QObject(parent)
    , m_impl(Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification"))
{
    invalidAppItemCached();
    connect(m_impl, &Dtk::Core::DConfig::valueChanged, this, [this] (const QString &key) {
        if (key == "appsInfo") {
            invalidAppItemCached();
        }
    });
}

void NotificationSetting::setAppAccessor(QAbstractListModel *model)
{
    m_appAccessor = model;
    QObject::connect(m_appAccessor, &QAbstractItemModel::rowsInserted, this, &NotificationSetting::onAppsChanged);
    QObject::connect(m_appAccessor, &QAbstractItemModel::rowsRemoved, this, &NotificationSetting::onAppsChanged);
}

void NotificationSetting::setAppValue(const QString &id, AppConfigItem item, const QVariant &value)
{
    auto info = appInfo(id);
    switch (item) {
    case EnableNotification: {
        info["enabled"] = value.toBool();
        break;
    }
    case EnablePreview: {
        info["enablePreview"] = value.toBool();
        break;
    }
    case EnableSound: {
        info["enableSound"] = value.toBool();
        break;
    }
    case ShowInNotificationCenter: {
        info["showInCenter"] = value.toBool();
        break;
    }
    case LockScreenShowNotification: {
        info["showInLockScreen"] = value.toBool();
        break;
    }
    default:
        break;
    }

    m_appsInfo[id] = info;
    m_impl->setValue("appsInfo", m_appsInfo);

    Q_EMIT appValueChanged(id, item, value);
}

QVariant NotificationSetting::appValue(const QString &id, AppConfigItem item)
{
    const auto app = appItem(id);
    switch (item) {
    case AppName: {
        return app.appName;
    }
    case AppIcon: {
        return app.appIcon;
    default:
        break;
    }
    }

    const auto info = appInfo(id);
    switch (item) {
    case EnableNotification: {
        return info.value("enabled", true);
    }
    case EnablePreview: {
        return info.value("enablePreview", true);
    }
    case EnableSound: {
        return info.value("enableSound", true);
    }
    case ShowInNotificationCenter: {
        return info.value("showInCenter", true);
    }
    case LockScreenShowNotification: {
        return info.value("showInLockScreen", true);
    }
    default:
        break;
    }

    return QVariant();
}

void NotificationSetting::setSystemValue(NotificationSetting::SystemConfigItem item, const QVariant &value)
{
    switch (item) {
    case DNDMode:
        m_impl->setValue("dndMode", value);
        break;
    case OpenByTimeInterval:
        m_impl->setValue("openByTimeInterval", value);
        break;
    case LockScreenOpenDNDMode:
        m_impl->setValue("lockScreenOpenDndMode", value);
        break;
    case StartTime:
        m_impl->setValue("startTime", value);
        break;
    case EndTime:
        m_impl->setValue("endTime", value);
        break;
    case CloseNotification:
        m_impl->setValue("notificationClosed", value);
        break;
    case MaxCount:
        m_impl->setValue("maxCount", value);
        break;
    default:
        return;
    }
    Q_EMIT systemValueChanged(item, value);
}

QVariant NotificationSetting::systemValue(NotificationSetting::SystemConfigItem item)
{
    QVariant result;
    switch (item) {
    case DNDMode:
        result = m_impl->value("dndMode", true);
        break;
    case LockScreenOpenDNDMode:
        result = m_impl->value("lockScreenOpenDndMode", false);
        break;
    case OpenByTimeInterval:
        result = m_impl->value("openByTimeInterval", true);
        break;
    case StartTime:
        result = m_impl->value("startTime", "07:00");
        break;
    case EndTime:
        result = m_impl->value("endTime", "22:00");
        break;
    case CloseNotification:
        result = m_impl->value("notificationClosed", false);
        break;
    case MaxCount:
        result = m_impl->value("maxCount", 2000);
        break;
    }

    return result;
}

QStringList NotificationSetting::apps() const
{
    QStringList ret;
    for (const auto &item : appItems()) {
        ret << item.id;
    }
    return ret;
}

NotificationSetting::AppItem NotificationSetting::appItem(const QString &id) const
{
    const auto infos = appItems();
    auto iter = std::find_if(infos.begin(), infos.end(), [id] (const AppItem &item) {
        return id == item.id;
    });
    if (iter != infos.end()) {
        return *iter;
    }

    return {};
}

QList<NotificationSetting::AppItem> NotificationSetting::appItems() const
{
    if (!m_appItems.isEmpty())
        return m_appItems;

    QList<NotificationSetting::AppItem> apps = appItemsImpl();
    const_cast<NotificationSetting *>(this)->m_appItems = apps;
    return m_appItems;
}

QList<NotificationSetting::AppItem> NotificationSetting::appItemsImpl() const
{
    if (!m_appAccessor)
        return {};

    QList<NotificationSetting::AppItem> apps;
    apps.reserve(m_appAccessor->rowCount());
    for (int i = 0; i < m_appAccessor->rowCount(); i++) {
        const auto index = m_appAccessor->index(i);
        const auto nodisplay = m_appAccessor->data(index, NoDisplayRole).toBool();
        if (nodisplay)
            continue;

        const auto desktopId = m_appAccessor->data(index, DesktopIdRole).toString();
        const auto icon = m_appAccessor->data(index, IconNameRole).toString();
        const auto name = m_appAccessor->data(index, NameRole).toString();

        NotificationSetting::AppItem item;
        item.id = desktopId;
        item.appIcon = icon;
        item.appName = name;
        apps << item;
    }
    return apps;
}

QVariantMap NotificationSetting::appInfo(const QString &id) const
{
    if (m_appsInfo.contains(InvalidApp)) {
        const_cast<NotificationSetting *>(this)->m_appsInfo = m_impl->value("appsInfo").toMap();
    }
    if (auto iter = m_appsInfo.find(id); iter != m_appsInfo.end()) {
        return iter.value().toMap();
    }
    return {};
}

void NotificationSetting::onAppsChanged()
{
    const auto old = m_appItems;
    const auto current = appItemsImpl();

    QList<NotificationSetting::AppItem> added;
    for (auto item : current) {
        const auto id = item.id;
        auto iter = std::find_if(old.begin(), old.end(), [id] (const NotificationSetting::AppItem &app) {
            return id == app.id;
        });
        if (iter == old.end()) {
            added << item;
        }
    }
    for (auto item : added) {
        qDebug() << "Application added" << item.id;
        Q_EMIT appAdded(item.id);
    }

    QList<NotificationSetting::AppItem> removed;
    for (auto item : old) {
        const auto id = item.id;
        auto iter = std::find_if(current.begin(), current.end(), [id] (const NotificationSetting::AppItem &app) {
            return id == app.id;
        });
        if (iter == current.end()) {
            removed << item;
        }
    }
    for (auto item : removed) {
        qDebug() << "Application removed" << item.id;
        Q_EMIT appRemoved(item.id);
    }

    m_appItems = current;
}

void NotificationSetting::invalidAppItemCached()
{
    m_appsInfo.clear();
    m_appsInfo[InvalidApp] = QVariant();
}

} // notification
