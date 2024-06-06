// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "constants.h"
#include "plugin.h"
#include "widgetplugin.h"
#include "pluginitem.h"

#include <QMenu>
#include <QPainter>
#include <QProcess>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <DGuiApplicationHelper>
#include <qglobal.h>


DGUI_USE_NAMESPACE

namespace dock {
WidgetPlugin::WidgetPlugin(PluginsItemInterface* pluginsItemInterface)
    : QObject()
    , m_pluginsItemInterface(pluginsItemInterface)
{
    QMetaObject::invokeMethod(this, [this](){
        m_pluginsItemInterface->init(this);
    });

    // TODO 需要给插件一个回调函数，以便插件通过回调函数告诉dock信息;并在回调函数中直接给出Json返回值
    // using MessageCallbackFunc = QString (*)(PluginsItemInterfaceV2 *, const QString&);
}

WidgetPlugin::~WidgetPlugin()
{
}

void WidgetPlugin::itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    qInfo() << "itemAdded:" << itemKey;
    auto flag = PluginItem::flags(itemInter);
    if (flag & Dock::Type_Quick) {
        PluginItem *item = new PluginItem(itemInter, QUICK_ITEM_KEY) ;
        Plugin::EmbedPlugin* plugin = Plugin::EmbedPlugin::get(item->windowHandle());
        initConnections(plugin);
        plugin->setPluginFlags(item->flags(itemInter));
        plugin->setItemKey(itemKey);
        plugin->setPluginType(TrayPluginType::Quick);
        Q_EMIT plugin->requestMessage("plugin test message");
        item->windowHandle()->hide();
        item->show();
        m_pluginItems << item;
    }
    if (flag & Dock::Attribute_Normal) {
        PluginItem *item = new PluginItem(itemInter, itemKey) ;
        Plugin::EmbedPlugin* plugin = Plugin::EmbedPlugin::get(item->windowHandle());
        initConnections(plugin);
        plugin->setPluginFlags(item->flags(itemInter));
        plugin->setItemKey(itemKey);
        plugin->setPluginType(TrayPluginType::Tray);
        Q_EMIT plugin->requestMessage("plugin test message");
        item->windowHandle()->hide();
        item->show();
        m_pluginItems << item;
    }

    // 模拟发送message request, 此调用应该在回调函数中
}

void WidgetPlugin::itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    if(m_pluginItem) m_pluginItem->update();

    auto widget = m_pluginsItemInterface->itemWidget(itemKey);
    if (widget) widget->update();

    auto quickPanel = m_pluginsItemInterface->itemWidget(QUICK_ITEM_KEY);
    if(quickPanel) quickPanel->update();

    auto popupWidget = m_pluginsItemInterface->itemPopupApplet(itemKey);
    if (popupWidget) popupWidget->update();

    auto tipsWidget = m_pluginsItemInterface->itemTipsWidget(itemKey);
    if (tipsWidget) tipsWidget->update();

}
void WidgetPlugin::itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    auto widget = m_pluginsItemInterface->itemWidget(itemKey);
    if(widget) widget->hide();

    auto quickPanel = m_pluginsItemInterface->itemWidget(QUICK_ITEM_KEY);
    if(quickPanel) quickPanel->hide();

    auto popupWidget = m_pluginsItemInterface->itemPopupApplet(itemKey);
    if(popupWidget) popupWidget->hide();

    auto tipsWidget = m_pluginsItemInterface->itemTipsWidget(itemKey);
    if(tipsWidget) tipsWidget->hide();

}

void WidgetPlugin::requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide)
{
}

void WidgetPlugin::requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey)
{
}

void WidgetPlugin::requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible)
{
    QWidget* appletWidget = itemInter->itemPopupApplet(itemKey);
    appletWidget->setFixedSize(400, 400);
    appletWidget->setParent(nullptr);
    appletWidget->show();
}

void WidgetPlugin::saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value)
{
}

const QVariant WidgetPlugin::getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback)
{
    return fallback;
}

void WidgetPlugin::removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList)
{
}


void WidgetPlugin::onDockPositionChanged(uint32_t position)
{
    qApp->setProperty(PROP_POSITION, position);
    m_pluginsItemInterface->positionChanged(static_cast<Dock::Position>(position));
}

void WidgetPlugin::onDockDisplayModeChanged(uint32_t displayMode)
{
    qApp->setProperty(PROP_DISPLAY_MODE, displayMode);
    m_pluginsItemInterface->displayModeChanged(static_cast<Dock::DisplayMode>(displayMode));
}

void WidgetPlugin::onDockEventMessageArrived(const QString &message)
{
    // TODO
}

Plugin::EmbedPlugin* WidgetPlugin::getPlugin(QWidget* widget)
{
    widget->setParent(nullptr);
    widget->winId();
    return Plugin::EmbedPlugin::get(widget->windowHandle());
}

void WidgetPlugin::initConnections(Plugin::EmbedPlugin *plugin)
{
    if (!plugin)
        return;

    connect(plugin, &Plugin::EmbedPlugin::dockColorThemeChanged, this, [](uint32_t type){
        DGuiApplicationHelper::instance()->setPaletteType(static_cast<DGuiApplicationHelper::ColorType>(type));
    }, Qt::UniqueConnection);

    connect(plugin, &Plugin::EmbedPlugin::dockPositionChanged, this, &WidgetPlugin::onDockPositionChanged, Qt::UniqueConnection);
    connect(plugin, &Plugin::EmbedPlugin::eventMessage, this, &WidgetPlugin::onDockEventMessageArrived, Qt::UniqueConnection);
}

}
