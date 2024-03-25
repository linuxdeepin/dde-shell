// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "constants.h"
#include "dockpanel.h"
#include "dockdbusproxy.h"

#include <QObject>

DS_BEGIN_NAMESPACE
namespace dock {
DockDBusProxy::DockDBusProxy(DockPanel* parent)
    : QObject(parent)
{
    connect(parent, &DockPanel::geometryChanged, this, &DockDBusProxy::FrontendWindowRectChanged);
    connect(parent, &DockPanel::positionChanged, this, &DockDBusProxy::PositionChanged);
    connect(parent, &DockPanel::hideModeChanged, this, &DockDBusProxy::hideModeChanged);
}

DockPanel* DockDBusProxy::parent() const
{
    return static_cast<DockPanel*>(QObject::parent());
}

QRect DockDBusProxy::geometry()
{
    return parent()->window() ? parent()->window()->geometry() : QRect();
}

QRect DockDBusProxy::frontendWindowRect()
{
    return parent()->frontendWindowRect();
}

Position DockDBusProxy::position()
{
    return parent()->position();
}

void DockDBusProxy::setPosition(Position position)
{
    parent()->setPosition(position);
}

HideMode DockDBusProxy::hideMode()
{
    return parent()->hideMode();
}

void DockDBusProxy::setHideMode(HideMode mode)
{
    parent()->setHideMode(mode);
}

HideState DockDBusProxy::hideState()
{
    return parent()->hideState();
}

QStringList DockDBusProxy::GetLoadedPlugins()
{
// TODO: implement this function
    return QStringList();
}

void DockDBusProxy::ReloadPlugins()
{
    parent()->ReloadPlugins();
}

void DockDBusProxy::callShow()
{
    parent()->callShow();
}

void DockDBusProxy::setItemOnDock(const QString &settingKey, const QString &itemKey, bool visible)
{
// TODO: implement this function
    Q_UNUSED(settingKey)
    Q_UNUSED(itemKey)
    Q_UNUSED(visible)
}

void DockDBusProxy::setPluginVisible(const QString &pluginName, bool visible)
{
// TODO: implement this function
    Q_UNUSED(pluginName)
    Q_UNUSED(visible)
}

bool DockDBusProxy::getPluginVisible(const QString &pluginName)
{
// TODO: implement this function
    Q_UNUSED(pluginName)
    return true;
}

QString DockDBusProxy::getPluginKey(const QString &pluginName)
{
// TODO: implement this function
    Q_UNUSED(pluginName)
    return QString();
}

void DockDBusProxy::resizeDock(int offset, bool dragging)
{
// TODO: implement this function
    Q_UNUSED(offset)
    Q_UNUSED(dragging)
}

}

DS_END_NAMESPACE
