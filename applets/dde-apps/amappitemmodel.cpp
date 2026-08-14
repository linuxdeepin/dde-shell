// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amappitemmodel.h"
#include "amappitem.h"
#include "appitemmodel.h"
#include "objectmanager1interface.h"
#include "trashmonitor.h"

#include <DUtil>

Q_LOGGING_CATEGORY(appsLog, "org.deepin.dde.shell.dde-apps.amappitemmodel")

namespace apps
{
AMAppItemModel::AMAppItemModel(QObject *parent)
    : AppItemModel(parent)
    , m_manager(new ObjectManager("org.desktopspec.ApplicationManager1", "/org/desktopspec/ApplicationManager1", QDBusConnection::sessionBus(), this))
    , m_trashMonitor(new TrashMonitor(this))
    , m_ready(false)
{
    qRegisterMetaType<ObjectInterfaceMap>();
    qDBusRegisterMetaType<ObjectInterfaceMap>();
    qRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<QStringMap>();
    qRegisterMetaType<QStringMap>();
    qRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<QDBusObjectPath>();

    connect(m_manager, &ObjectManager::InterfacesAdded, this, [this](const QDBusObjectPath &objPath, ObjectInterfaceMap interfacesAndProperties) {
        auto desktopId = DUtil::unescapeFromObjectPath(objPath.path().split('/').last());
        if (!match(index(0, 0), AppItemModel::DesktopIdRole, desktopId, 1, Qt::MatchExactly).isEmpty()) {
            qCWarning(appsLog()) << "desktopId: " << desktopId << " already contains";
            return;
        }
        appendRow(new AMAppItem(objPath, interfacesAndProperties));
        updateTrashIcon();
    });

    connect(m_trashMonitor, &TrashMonitor::emptyChanged, this, &AMAppItemModel::updateTrashIcon);
    connect(this, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
        const auto trashIndex = match(index(0, 0), AppItemModel::DesktopIdRole,
                                      QStringLiteral("dde-trash"), 1, Qt::MatchExactly).value(0);
        if (trashIndex.isValid() && trashIndex.row() >= topLeft.row() && trashIndex.row() <= bottomRight.row()
            && (roles.isEmpty() || roles.contains(AppItemModel::IconNameRole))) {
            updateTrashIcon();
        }
    });

    connect(m_manager, &ObjectManager::InterfacesRemoved, this, [this](const QDBusObjectPath &objPath, const QStringList &interfaces) {
        Q_UNUSED(interfaces)
        auto desktopId = DUtil::unescapeFromObjectPath(objPath.path().split('/').last());
        auto res = match(index(0, 0), AppItemModel::DesktopIdRole, desktopId, 1, Qt::MatchExactly);
        if (res.isEmpty()) {
            qCWarning(appsLog()) << "failed find desktopId: " << desktopId;
            return;
        }
        removeRow(res.first().row());
    });

    auto watcher = new QDBusPendingCallWatcher(m_manager->GetManagedObjects(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        QDBusPendingReply<ObjectMap> reply = *watcher;
        watcher->deleteLater();
        if (reply.isError()) {
            qCWarning(appsLog) << "Failed to load applications from ApplicationManager:" << reply.error();
            return;
        }

        const auto apps = reply.value();
        for (auto app = apps.cbegin(); app != apps.cend(); ++app) {
            const auto path = app.key();
            if (path.path().isEmpty())
                continue;

            const auto desktopId = DUtil::unescapeFromObjectPath(path.path().split('/').last());
            if (!match(index(0, 0), AppItemModel::DesktopIdRole, desktopId, 1, Qt::MatchExactly).isEmpty())
                continue;
            appendRow(new AMAppItem(path, app.value()));
        }


        updateTrashIcon();

        m_ready = true;
        Q_EMIT readyChanged(true);
        qCDebug(appsLog) << "AMAppItemModel is now ready with apps counts:" << rowCount();
    });
}

bool AMAppItemModel::ready() const
{
    return m_ready;
}

AMAppItem * AMAppItemModel::appItem(const QString &id)
{
    for (int i = 0; i < rowCount(); i++) {
        auto app = item(i);
        if (app->data(AppItemModel::DesktopIdRole).toString() == id)
            return static_cast<AMAppItem *>(app);
    }
    return nullptr;
}

void AMAppItemModel::updateTrashIcon()
{
    auto *trash = appItem(QStringLiteral("dde-trash"));
    if (!trash)
        return;

    const QString iconName = m_trashMonitor->isEmpty()
            ? QStringLiteral("user-trash")
            : QStringLiteral("user-trash-full");
    if (trash->appIconName() != iconName)
        trash->setAppIconName(iconName);
}

}
