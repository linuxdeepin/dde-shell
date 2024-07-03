// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trayitem.h"

#include "pluginfactory.h"
#include "docksettings.h"
#include "traysettings.h"

DGUI_USE_NAMESPACE

const QString DockQuickPlugins = "Dock_Quick_Plugins";
const int attribute_CanSetting = 0x800;

namespace dock {

TrayItem::TrayItem(QObject *parent)
    : DApplet(parent)
{

}

QAbstractItemModel *TrayItem::trayPluginModel() const
{
    return m_trayPluginModel;
}

void TrayItem::setTrayPluginModel(QAbstractItemModel *newTrayPluginModel)
{
    if (m_trayPluginModel == newTrayPluginModel)
        return;
    m_trayPluginModel = newTrayPluginModel;
    emit trayPluginModelChanged();
}

DockItemInfos TrayItem::dockItemInfos()
{
    const auto targetModel = m_trayPluginModel;
    if (!targetModel)
        return DockItemInfos{};

    const auto roleNames = targetModel->roleNames();
    const auto modelDataRole = roleNames.key("shellSurface", -1);
    if (modelDataRole < 0)
        return DockItemInfos{};

    DockItemInfos itemInfos;
    for (int i = 0; i < targetModel->rowCount(); i++) {
        const auto index = targetModel->index(i, 0);
        const auto item = index.data(modelDataRole).value<QObject *>();
        if (!item)
            return DockItemInfos{};
        if (!(item->property("pluginFlags").toInt() & attribute_CanSetting)) {
            continue;
        }
        DockItemInfo itemInfo;
        itemInfo.name = item->property("pluginId").toString();
        itemInfo.displayName = item->property("displayName").toString();
        itemInfo.itemKey = item->property("itemKey").toString();
        itemInfo.settingKey = DockQuickPlugins;
        itemInfo.dccIcon = item->property("dccIcon").toString();
        itemInfo.visible = TraySettings::instance()->trayItemIsOnDock(itemInfo.name + "::" + itemInfo.itemKey);
        itemInfos << itemInfo;
    }
    m_itemInfos = itemInfos;
    return m_itemInfos;
}

void TrayItem::setItemOnDock(const QString &settingKey, const QString &itemKey, bool visible)
{
    Q_UNUSED(settingKey)
    QString pluginId;
    for (const DockItemInfo &itemInfo : m_itemInfos) {
        if (itemInfo.itemKey == itemKey) {
            pluginId = itemInfo.name;
            break;
        }
    }
    visible ? TraySettings::instance()->addTrayItemOnDock(pluginId + "::" + itemKey) :
              TraySettings::instance()->removeTrayItemOnDock(pluginId + "::" + itemKey);
}

D_APPLET_CLASS(TrayItem)
}


#include "trayitem.moc"
