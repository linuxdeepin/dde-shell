#include "quickpluginitem.h"
#include "constants.h"
#include "plugin.h"

QuickPluginItem::QuickPluginItem(PluginsItemInterface *pluginInterface, const QString &itemKey, QWidget *parent)
    : PluginItem(pluginInterface, itemKey, parent)
{
}

QWidget *QuickPluginItem::centralWidget()
{
    if (pluginsItemInterface()) {
        return pluginsItemInterface()->itemWidget(Dock::QUICK_ITEM_KEY);
    }
    return nullptr;
}

