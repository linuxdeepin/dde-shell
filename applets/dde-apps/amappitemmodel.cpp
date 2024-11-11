// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amappitemmodel.h"
#include "amappitem.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "objectmanager1interface.h"

#include <DUtil>
#include <QtConcurrent>
#include <tuple>

Q_LOGGING_CATEGORY(appsLog, "dde.shell.dde-apps.amappitemmodel")

namespace apps
{
AMAppItemModel::AMAppItemModel(QObject *parent)
    : AppItemModel(parent)
    , m_manager(new ObjectManager("org.desktopspec.ApplicationManager1", "/org/desktopspec/ApplicationManager1", QDBusConnection::sessionBus()))
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
        if (!match(index(0, 0), AppItemModel::DesktopIdRole, desktopId).isEmpty()) {
            qCWarning(appsLog()) << "desktopId: " << desktopId << " already contains";
            return;
        }
        appendRow(new AMAppItem(objPath, interfacesAndProperties));
    });

    connect(m_manager, &ObjectManager::InterfacesRemoved, this, [this](const QDBusObjectPath &objPath, const QStringList &interfaces) {
        auto desktopId = DUtil::unescapeFromObjectPath(objPath.path().split('/').last());
        auto res = match(index(0, 0), AppItemModel::DesktopIdRole, desktopId);
        if (res.isEmpty()) {
            qCWarning(appsLog()) << "failed find desktopId: " << desktopId;
            return;
        }
        removeRow(res.first().row());
    });

    // load static desktop info from am
    auto future = QtConcurrent::run([this]() {
        auto apps = m_manager->GetManagedObjects().value();

        for (auto app = apps.cbegin(); app != apps.cend(); app++) {
            auto path = app.key();
            if (!path.path().isEmpty()) {
                auto c = new AMAppItem(path, app.value());
                appendRow(c);
            }
        }
    });
}
}