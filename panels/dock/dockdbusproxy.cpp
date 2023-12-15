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
    connect(parent, &DockPanel::displayModeChanged, this, &DockDBusProxy::displayModeChanged);
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

DisplayMode DockDBusProxy::displayMode()
{
    return parent()->displayMode();
}

void DockDBusProxy::setDisplayMode(DisplayMode mode)
{
    parent()->setDisplayMode(mode);
}

HideState DockDBusProxy::hideState()
{
    return HideState::Unknown;
}

}

DS_END_NAMESPACE
