// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockplugin.h"

#include <QMap>

namespace dock {
class DockPluginPrivate
{
public:
    explicit DockPluginPrivate(QWindow* window)
        : parentWindow(window)
    {}

    QWindow* parentWindow;
    DockPlugin::PluginType pluginType = DockPlugin::PluginType::Quick;
    QString pluginId;
    QString itemKey;
    QString dccIcon;
    QString contextMenu;
    int pluginFlags;
};

DockPlugin::DockPlugin(QWindow* window)
    : QObject(window)
    , d(new DockPluginPrivate(window))
{}

DockPlugin::~DockPlugin()
{}

DockPlugin::PluginType DockPlugin::pluginType() const
{
    return d->pluginType;
}

void DockPlugin::setPluginType(PluginType type)
{
    if (type != d->pluginType) {
        d->pluginType = type;
        Q_EMIT pluginTypeChanged();
    }
}

QString DockPlugin::dccIcon() const
{
    return d->dccIcon;
}

void DockPlugin::setDCCIcon(const QString& dccIcon)
{
    if (dccIcon != d->dccIcon) {
        d->dccIcon = dccIcon;
        Q_EMIT dccIconChanged();
    }
}

QString DockPlugin::pluginId() const
{
    return d->pluginId;
}

void DockPlugin::setPluginId(const QString& id)
{
    if (id != d->pluginId) {
        d->pluginId = id;
        Q_EMIT pluginIdChanged();
    }
}

QString DockPlugin::itemKey() const
{
    return d->itemKey;
}

void DockPlugin::setItemKey(const QString& itemKey)
{
    if (itemKey != d->itemKey) {
        d->itemKey = itemKey;
        Q_EMIT itemKeyChanged();
    }
}

QString DockPlugin::contextMenu() const
{
    return d->contextMenu;
}

void DockPlugin::setContextMenu(const QString& contextMenu)
{
    if (contextMenu != d->contextMenu) {
        d->contextMenu = contextMenu;
        Q_EMIT contextMenuChanged();
    }
}

int DockPlugin::pluginFlags() const
{
    return d->pluginFlags;
}

void DockPlugin::setPluginFlags(int flags)
{
    if (flags != d->pluginFlags) {
        d->pluginFlags = flags;
        Q_EMIT pluginFlagsChnaged();
    }
}

static QMap<QWindow*, DockPlugin*> s_map;
DockPlugin* DockPlugin::get(QWindow* window)
{
    auto plugin = s_map.value(window);
    if (!plugin) {
        plugin = new DockPlugin(window);
        s_map.insert(window, plugin);
    }

    return plugin;
}

DockPlugin* DockPlugin::qmlAttachedProperties(QObject *object)
{
    auto window = qobject_cast<QWindow*>(object);
    if (window)
        return get(window);
    return nullptr;
}
}
