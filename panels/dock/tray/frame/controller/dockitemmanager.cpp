// Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dockitemmanager.h"
#include "pluginsitem.h"
#include "utils.h"
#include "quicksettingcontroller.h"

#include <QDebug>
#include <QGSettings>

#include <DApplication>

DockItemManager *DockItemManager::INSTANCE = nullptr;
const QGSettings *DockItemManager::m_appSettings = Utils::ModuleSettingsPtr("app");
const QGSettings *DockItemManager::m_activeSettings = Utils::ModuleSettingsPtr("activeapp");
const QGSettings *DockItemManager::m_dockedSettings = Utils::ModuleSettingsPtr("dockapp");

DockItemManager::DockItemManager(QObject *parent)
    : QObject(parent)
    , m_loadFinished(false)
{
    // 托盘区域和插件区域 由DockPluginsController获取
    QuickSettingController *quickController = QuickSettingController::instance();
    connect(quickController, &QuickSettingController::pluginInserted, this, [ = ](PluginsItemInterface *itemInter, const QuickSettingController::PluginAttribute pluginAttr) {
        if (pluginAttr != QuickSettingController::PluginAttribute::Fixed)
            return;

        m_pluginItems << itemInter;
        pluginItemInserted(quickController->pluginItemWidget(itemInter));
    });

    connect(quickController, &QuickSettingController::pluginRemoved, this, &DockItemManager::onPluginItemRemoved);
    connect(quickController, &QuickSettingController::pluginUpdated, this, &DockItemManager::onPluginUpdate);
    connect(quickController, &QuickSettingController::pluginLoaderFinished, this, &DockItemManager::onPluginLoadFinished, Qt::QueuedConnection);

    DApplication *app = qobject_cast<DApplication *>(qApp);
    if (app) {
        connect(app, &DApplication::iconThemeChanged, this, &DockItemManager::refreshItemsIcon);
    }

    connect(qApp, &QApplication::aboutToQuit, this, &QObject::deleteLater);

    // 读取已经加载的固定区域插件
    QList<PluginsItemInterface *> plugins = quickController->pluginItems(QuickSettingController::PluginAttribute::Fixed);
    for (PluginsItemInterface *plugin : plugins) {
        m_pluginItems << plugin;
        pluginItemInserted(quickController->pluginItemWidget(plugin));
    }

    // 刷新图标
    QMetaObject::invokeMethod(this, &DockItemManager::refreshItemsIcon, Qt::QueuedConnection);
}

DockItemManager *DockItemManager::instance(QObject *parent)
{
    if (!INSTANCE)
        INSTANCE = new DockItemManager(parent);

    return INSTANCE;
}

const QList<QPointer<DockItem>> DockItemManager::itemList() const
{
    return m_itemList;
}

void DockItemManager::refreshItemsIcon()
{
    for (auto item : m_itemList) {
        if (item.isNull())
            continue;

        item->refreshIcon();
        item->update();
    }
}

/**
 * @brief 将插件的参数(Order, Visible, etc)写入gsettings
 * 自动化测试需要通过dbus(GetPluginSettings)获取这些参数
 */
void DockItemManager::updatePluginsItemOrderKey()
{
    int index = 0;
    for (auto item : m_itemList) {
        if (item.isNull() || item->itemType() != DockItem::Plugins)
            continue;
        static_cast<PluginsItem *>(item.data())->setItemSortKey(++index);
    }

    // 固定区域插件排序
    index = 0;
    for (auto item : m_itemList) {
        if (item.isNull() || item->itemType() != DockItem::FixedPlugin)
            continue;
        static_cast<PluginsItem *>(item.data())->setItemSortKey(++index);
    }
}

void DockItemManager::itemMoved(DockItem *const sourceItem, DockItem *const targetItem)
{
    Q_ASSERT(sourceItem != targetItem);

    const DockItem::ItemType moveType = sourceItem->itemType();
    const DockItem::ItemType replaceType = targetItem->itemType();

    // app move
    if (moveType == DockItem::App || moveType == DockItem::Placeholder)
        if (replaceType != DockItem::App)
            return;

    // plugins move
    if (moveType == DockItem::Plugins || moveType == DockItem::TrayPlugin)
        if (replaceType != DockItem::Plugins && replaceType != DockItem::TrayPlugin)
            return;

    const int moveIndex = m_itemList.indexOf(sourceItem);
    const int replaceIndex = m_itemList.indexOf(targetItem);

    m_itemList.removeAt(moveIndex);
    m_itemList.insert(replaceIndex, sourceItem);

    // update plugins sort key if order changed
    if (moveType == DockItem::Plugins || replaceType == DockItem::Plugins
            || moveType == DockItem::TrayPlugin || replaceType == DockItem::TrayPlugin
            || moveType == DockItem::FixedPlugin || replaceType == DockItem::FixedPlugin) {
        updatePluginsItemOrderKey();
    }
}

void DockItemManager::manageItem(DockItem *item)
{
    connect(item, &DockItem::requestRefreshWindowVisible, this, &DockItemManager::requestRefershWindowVisible, Qt::UniqueConnection);
    connect(item, &DockItem::requestWindowAutoHide, this, &DockItemManager::requestWindowAutoHide, Qt::UniqueConnection);
}

void DockItemManager::pluginItemInserted(PluginsItem *item)
{
    manageItem(item);

    DockItem::ItemType pluginType = item->itemType();

    // find first plugins item position
    int firstPluginPosition = -1;
    for (int i(0); i != m_itemList.size(); ++i) {
        DockItem::ItemType type = m_itemList[i]->itemType();
        if (type != pluginType)
            continue;

        firstPluginPosition = i;
        break;
    }

    if (firstPluginPosition == -1)
        firstPluginPosition = m_itemList.size();

    // find insert position
    int insertIndex = 0;
    const int itemSortKey = item->itemSortKey();
    if (itemSortKey == -1 || firstPluginPosition == -1) {
        insertIndex = m_itemList.size();
    } else if (itemSortKey == 0) {
        insertIndex = firstPluginPosition;
    } else {
        insertIndex = m_itemList.size();
        for (int i(firstPluginPosition + 1); i != m_itemList.size() + 1; ++i) {
            PluginsItem *pItem = static_cast<PluginsItem *>(m_itemList[i - 1].data());
            Q_ASSERT(pItem);

            const int sortKey = pItem->itemSortKey();
            if (pluginType == DockItem::FixedPlugin) {
                if (sortKey != -1 && itemSortKey > sortKey)
                    continue;
                insertIndex = i - 1;
                break;
            }
            if (sortKey != -1 && itemSortKey > sortKey && pItem->itemType() != DockItem::FixedPlugin)
                continue;
            insertIndex = i - 1;
            break;
        }
    }

    m_itemList.insert(insertIndex, item);
    if(pluginType == DockItem::FixedPlugin)
        insertIndex ++;

    if (!Utils::SettingValue(QString("com.deepin.dde.dock.module.") + item->pluginName(), QByteArray(), "enable", true).toBool())
        item->setVisible(false);
    else
        item->setVisible(true);

    emit itemInserted(insertIndex - firstPluginPosition, item);
}

void DockItemManager::onPluginItemRemoved(PluginsItemInterface *itemInter)
{
    if (!m_pluginItems.contains(itemInter))
        return;

    PluginsItem *item = QuickSettingController::instance()->pluginItemWidget(itemInter);
    item->hidePopup();
    item->hide();

    emit itemRemoved(item);

    m_itemList.removeOne(item);

    if (m_loadFinished) {
        updatePluginsItemOrderKey();
    }
}

void DockItemManager::onPluginUpdate(PluginsItemInterface *itemInter)
{
    if (!m_pluginItems.contains(itemInter))
        return;

    Q_EMIT itemUpdated(QuickSettingController::instance()->pluginItemWidget(itemInter));
}

void DockItemManager::onPluginLoadFinished()
{
    updatePluginsItemOrderKey();
    m_loadFinished = true;
}

