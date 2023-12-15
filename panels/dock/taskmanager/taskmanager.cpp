// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"

#include "dsglobal.h"
#include "globals.h"
#include "taskmanager.h"
#include "appitemmodel.h"
#include "pluginfactory.h"
#include "abstractwindow.h"
#include "taskmanageradaptor.h"
#include "desktopfileamparser.h"
#include "taskmanagersettings.h"
#include "waylandwindowmonitor.h"
#include "desktopfilenoneparser.h"
#include "abstractwindowmonitor.h"
#include "desktopfileparserfactory.h"

#include <QStringLiteral>
#include <QGuiApplication>

#ifdef BUILD_WITH_X11
#include "x11windowmonitor.h"
#endif

Q_LOGGING_CATEGORY(taskManagerLog, "dde.shell.dock.taskmanager", QtInfoMsg)

#define Settings TaskManagerSettings::instance()

DS_BEGIN_NAMESPACE
namespace dock {

TaskManager::TaskManager(QObject* parent)
    : DApplet(parent)
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

    connect(AppItemModel::instance(), &AppItemModel::appItemAdded, this, &TaskManager::appItemsChanged);
    connect(AppItemModel::instance(), &AppItemModel::appItemRemoved, this, &TaskManager::appItemsChanged);

    connect(Settings, &TaskManagerSettings::allowedForceQuitChanged, this, &TaskManager::allowedForceQuitChanged);
    connect(Settings, &TaskManagerSettings::windowSplitChanged, this, &TaskManager::windowSplitChanged);
}

bool TaskManager::load()
{
    loadDockedAppItems();
    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        m_windowMonitor.reset(new WaylandWindowMonitor());
    }

#ifdef BUILD_WITH_X11
    else if (QStringLiteral("xcb") == platformName) {
        m_windowMonitor.reset(new X11WindowMonitor());
    }
#endif

    connect(m_windowMonitor.get(), &AbstractWindowMonitor::windowAdded, this, &TaskManager::handleWindowAdded);
    return true;
}

bool TaskManager::init()
{
    auto adaptor = new TaskManagerAdaptor(this);
    QDBusConnection::sessionBus().registerService("org.deepin.ds.Dock.TaskManager");
    QDBusConnection::sessionBus().registerObject("/org/deepin/ds/Dock/TaskManager", "org.deepin.ds.Dock.TaskManager", this);

    DApplet::init();
    connect(AppItemModel::instance(), &AppItemModel::appItemAdded, this, &TaskManager::appItemsChanged);
    connect(AppItemModel::instance(), &AppItemModel::appItemRemoved, this, &TaskManager::appItemsChanged);
    if (m_windowMonitor)
        m_windowMonitor->start();
    return true;
}

AppItemModel* TaskManager::dataModel()
{
    return AppItemModel::instance();
}

void TaskManager::handleWindowAdded(QPointer<AbstractWindow> window)
{
    if (!window || window->shouldSkip() || window->getAppItem() != nullptr) return;
    auto desktopfile = DesktopfileParserFactory<DesktopFileAMParser>::createByWindow(window);
    if(!desktopfile) desktopfile = DesktopfileParserFactory<DesktopFileNoneParser>::createByWindow(window);

    auto appitem = desktopfile->getAppItem();

    if (appitem.isNull() || (appitem->hasWindow() && windowSplit())) {
        auto id = windowSplit() ? QString("%1@%2").arg(desktopfile->id()).arg(window->id()) : desktopfile->id();
        appitem = new AppItem(id);
    }

    appitem->appendWindow(window);
    appitem->setDesktopFileParser(desktopfile);

    AppItemModel::instance()->addAppItem(appitem);
}

void TaskManager::clickItem(const QString& itemId)
{
    qCInfo(taskManagerLog) << "Item" << itemId << "is clicked.";
    auto appitem = AppItemModel::instance()->getAppItemById(itemId);
    if (appitem->hasWindow())
        appitem->active();
    else
        appitem->launch();
}

void TaskManager::clickItemMenu(const QString& itemId, const QString& menuId)
{
    qCInfo(taskManagerLog) << "Item" << itemId << "menu" << menuId << "is clicked.";
    auto appitem = AppItemModel::instance()->getAppItemById(itemId);
    if(!appitem) return;
    if (menuId == DOCK_ACTION_ALLWINDOW) {
        m_windowMonitor->presentWindows(appitem->windows());
        return;
    }
    if (appitem) appitem->handleMenu(menuId);
}

void TaskManager::loadDockedAppItems()
{
    for (auto id : DesktopFileAMParser::loadDockedDesktopfile()) {
        auto desktopfile = DesktopfileParserFactory<DesktopFileAMParser>::createById(id);
        auto appitem = new AppItem(id);
        appitem->setDesktopFileParser(desktopfile);
        AppItemModel::instance()->addAppItem(appitem);
    }
}

bool TaskManager::allowForceQuit()
{
    return Settings->isAllowedForceQuit();
}

bool TaskManager::windowSplit()
{
    return Settings->isWindowSplit();
}

D_APPLET_CLASS(TaskManager)
}

DS_END_NAMESPACE
#include "taskmanager.moc"
