// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appsapplet.h"
#include "appitem.h"
#include "appitemmodel.h"
#include "desktopinfoam.h"
#include "pluginfactory.h"
#include "appslaunchtimes.h"

#include "objectmanager1interface.h"

#include <QtConcurrent>
#include <DUtil>
#include <qlogging.h>

Q_LOGGING_CATEGORY(appsLog, "dde.shell.appsmodel")

namespace apps {
static ObjectManager am("org.desktopspec.ApplicationManager1", "/org/desktopspec/ApplicationManager1", QDBusConnection::sessionBus());

AppsApplet::AppsApplet(QObject *parent)
    : DApplet(parent)
    , m_model(new AppItemModel(this))
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
}

AppsApplet::~AppsApplet()
{

}

bool AppsApplet::load()
{
    connect(&am, &ObjectManager::InterfacesAdded, this, [this](const QDBusObjectPath &objPath, ObjectInterfaceMap interfacesAndProperties) {
        auto desktopId = DUtil::unescapeFromObjectPath(objPath.path().split('/').last());
        if (!m_model->match(m_model->index(0), AppItemModel::DesktopIdRole, desktopId).isEmpty()) {
            qCWarning(appsLog()) << "desktopId: " << desktopId << " already contains";
            return;
        }

        DesktopInfoAM* desktop = new DesktopInfoAM(objPath.path(), interfacesAndProperties);
        AppItem* item = new AppItem(desktop);
        m_model->addAppItems({ item });
    });

    connect(&am, &ObjectManager::InterfacesRemoved, this, [this](const QDBusObjectPath &objPath, const QStringList &interfaces) {
        auto desktopId = DUtil::unescapeFromObjectPath(objPath.path().split('/').last());
        auto res = m_model->match(m_model->index(0), AppItemModel::DesktopIdRole, desktopId);
        if ( res.isEmpty()) {
            qCWarning(appsLog()) << "failed find desktopId: " << desktopId;
            return;
        }

        auto appItem = static_cast<AppItem*>(res.first().internalPointer());
        m_model->removeAppItems({appItem});
        AppsLaunchTimesHelper::instance()->setLaunchTimesFor(desktopId, 0);
    });

    // load static desktop info from am
    auto future = QtConcurrent::run([this](){
        QList<AppItem*> appItems;

        auto apps = am.GetManagedObjects().value();

        for (auto app = apps.cbegin(); app != apps.cend(); app++) {
            auto path = app.key().path();
            if (!path.isEmpty()) {
                auto desktopInfo = new DesktopInfoAM(path, app.value());
                auto appitem = new AppItem(desktopInfo);
                appItems.append(appitem);
            }
        }

        m_model->addAppItems(appItems);
    });

    return true;
}

QAbstractListModel* AppsApplet::appModel() const
{
    return m_model;
}

D_APPLET_CLASS(AppsApplet)
}

#include "appsapplet.moc"
