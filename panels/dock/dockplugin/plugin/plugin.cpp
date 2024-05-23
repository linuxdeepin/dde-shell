// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plugin.h"

#include <QMap>
#include <cstdint>

namespace Plugin {
class EmbemdPluginPrivate
{
public:
    explicit EmbemdPluginPrivate(QWindow* window)
        : parentWindow(window)
    {}

    QWindow* parentWindow;
    QString pluginId;
    QString itemKey;
    int pluginFlags;
    int pluginType;
    uint32_t pluginOrder;
};

EmbemdPlugin::EmbemdPlugin(QWindow* window)
    : QObject(window)
    , d(new EmbemdPluginPrivate(window))
{

}

EmbemdPlugin::~EmbemdPlugin()
{
    d.reset(nullptr);
}

QString EmbemdPlugin::pluginId() const
{
    return d->pluginId;
}

QString EmbemdPlugin::itemKey() const
{
    return d->itemKey;
}

int EmbemdPlugin::pluginType() const
{
    return d->pluginType;
}

int EmbemdPlugin::pluginFlags() const
{
    return d->pluginFlags;
}

uint32_t EmbemdPlugin::pluginOrder() const
{
    return d->pluginOrder;
}

void EmbemdPlugin::setPluginId(const QString& pluginid)
{
    if (d->pluginId == pluginid) {
        return;
    }

    d->pluginId = pluginid;
    Q_EMIT pluginIdChanged();
}

void EmbemdPlugin::setItemKey(const QString& itemkey)
{
    if (d->itemKey == itemkey) {
        return;
    }

    d->itemKey = itemkey;
    Q_EMIT itemKeyChanged();
}

void EmbemdPlugin::setPluginType(int type)
{
    if (d->pluginType == type) {
        return;
    }

    d->pluginType = type;
    Q_EMIT pluginTypeChanged();
}

void EmbemdPlugin::setPluginFlags(int flags)
{
    if (d->pluginFlags == flags) {
        return;
    }

    d->pluginFlags = flags;
    Q_EMIT pluginFlagsChanged();
}

void EmbemdPlugin::setPluginOrder(uint32_t order)
{
    if (d->pluginOrder == order) {
        return;
    }

    d->pluginOrder = order;
    Q_EMIT pluginOrderChanged();
}

static QMap<QWindow*, EmbemdPlugin*> s_map;
EmbemdPlugin* EmbemdPlugin::get(QWindow* window)
{
    auto plugin = s_map.value(window);
    if (!plugin) {
        plugin = new EmbemdPlugin(window);
        s_map.insert(window, plugin);
    }

    return plugin;
}

bool EmbemdPlugin::contains(QWindow *window)
{
    return s_map.keys().contains(window);
}

class PluginPopupPrivate
{
public:
    explicit PluginPopupPrivate(QWindow* window)
        : parentWindow(window)
    {}

    QWindow* parentWindow;
    QString pluginId;
    QString itemKey;
    int pluginFlags;
    int popupType;
    int x;
    int y;
};

PluginPopup::PluginPopup(QWindow* window)
    : QObject(window)
    , d(new PluginPopupPrivate(window))
{

}

PluginPopup::~PluginPopup()
{
    d.reset(nullptr);
}

QString PluginPopup::pluginId() const
{
    return d->pluginId;
}

QString PluginPopup::itemKey() const
{
    return d->itemKey;
}

int PluginPopup::popupType() const
{
    return d->popupType;
}

void PluginPopup::setPluginId(const QString& pluginid)
{
    if (d->pluginId == pluginid) {
        return;
    }

    d->pluginId = pluginid;
    Q_EMIT pluginIdChanged();
}

void PluginPopup::setItemKey(const QString& itemkey)
{
    if (d->itemKey == itemkey) {
        return;
    }

    d->itemKey = itemkey;
    Q_EMIT itemKeyChanged();
}

void PluginPopup::setPopupType(const int& type)
{
    if (d->popupType == type) {
        return;
    }

    d->popupType = type;
    Q_EMIT popupTypeChanged();
}

int PluginPopup::x() const
{
    return d->x;
}

int PluginPopup::y() const
{
    return d->y;
}

void PluginPopup::setX(const int& x)
{
    if (d->x == x) {
        return;
    }

    d->x = x;
    Q_EMIT xChanged();
}

void PluginPopup::setY(const int& y)
{
    if (d->y == y) {
        return;
    }

    d->y = y;
    Q_EMIT yChanged();
}

static QMap<QWindow*, PluginPopup*> s_popupMap;
PluginPopup* PluginPopup::get(QWindow* window)
{
    auto popup = s_popupMap.value(window);
    if (!popup) {
        popup = new PluginPopup(window);
        s_popupMap.insert(window, popup);
    }

    return popup;
}

bool PluginPopup::contains(QWindow *window)
{
    return s_popupMap.keys().contains(window);
}
}
