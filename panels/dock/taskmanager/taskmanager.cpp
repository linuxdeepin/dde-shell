// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"

#include "dsglobal.h"
#include "globals.h"
#include "itemmodel.h"
#include "taskmanager.h"
#include "pluginfactory.h"
#include "abstractwindow.h"
#include "taskmanageradaptor.h"
#include "desktopfileamparser.h"
#include "taskmanagersettings.h"
#include "waylandwindowmonitor.h"
#include "abstractwindowmonitor.h"
#include "desktopfileparserfactory.h"

#include <QStringLiteral>
#include <QGuiApplication>

#ifdef BUILD_WITH_X11
#include "x11windowmonitor.h"
#endif

Q_LOGGING_CATEGORY(taskManagerLog, "dde.shell.dock.taskmanager", QtInfoMsg)

#define Settings TaskManagerSettings::instance()

#define DESKTOPFILEFACTORY DesktopfileParserFactory<    \
                            DesktopFileAMParser,        \
                            DesktopfileAbstractParser   \
                        >

DS_BEGIN_NAMESPACE
namespace dock {

TaskManager::TaskManager(QObject* parent)
    : DContainment(parent)
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

    connect(ItemModel::instance(), &ItemModel::itemAdded, this, &TaskManager::itemsChanged);
    connect(ItemModel::instance(), &ItemModel::itemRemoved, this, &TaskManager::itemsChanged);

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
    if (m_windowMonitor)
        m_windowMonitor->start();
    return true;
}

ItemModel* TaskManager::dataModel()
{
    return ItemModel::instance();
}

void TaskManager::handleWindowAdded(QPointer<AbstractWindow> window)
{
    if (!window || window->shouldSkip() || window->getAppItem() != nullptr) return;
    auto desktopfile = DESKTOPFILEFACTORY::createByWindow(window);

    auto appitem = desktopfile->getAppItem();

    if (appitem.isNull() || (appitem->hasWindow() && windowSplit())) {
        auto id = windowSplit() ? QString("%1@%2").arg(desktopfile->id()).arg(window->id()) : desktopfile->id();
        appitem = new AppItem(id);
    }

    appitem->appendWindow(window);
    appitem->setDesktopFileParser(desktopfile);

    ItemModel::instance()->addItem(appitem);
}

void TaskManager::clickItem(const QString& itemId, const QString& menuId)
{
    auto item = ItemModel::instance()->getItemById(itemId);
    if(!item) return;

    if (menuId == DOCK_ACTION_ALLWINDOW) {
        QList<uint32_t> windowIds;
        auto windows = item->data().toStringList();
        std::transform(windows.begin(), windows.end(), std::back_inserter(windowIds), [](const QString &windowId) {
            return windowId.toUInt();
        });

        m_windowMonitor->presentWindows(windowIds);
        return;
    }

    item->handleClick(menuId);
}

void TaskManager::showItemPreview(const QString &itemId, QObject* relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    auto item = ItemModel::instance()->getItemById(itemId).get();
    if (!item) return;

    QPointer<AppItem> pItem = reinterpret_cast<AppItem*>(item);
    if (pItem.isNull()) return;

    m_windowMonitor->showItemPreview(pItem, relativePositionItem, previewXoffset, previewYoffset, direction);
}

void TaskManager::hideItemPreview()
{
    m_windowMonitor->hideItemPreview();
}

void TaskManager::loadDockedAppItems()
{
    // TODO: add support for group and dir type
    for (const auto& appValueRef : TaskManagerSettings::instance()->dockedDesktopFiles()) {
        auto app = appValueRef.toObject();
        auto appid = app.value("id").toString();
        auto type = app.value("type").toString();
        auto desktopfile = DESKTOPFILEFACTORY::createById(appid, type);

        auto appitem = new AppItem(appid);
        appitem->setDesktopFileParser(desktopfile);
        ItemModel::instance()->addItem(appitem);
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
