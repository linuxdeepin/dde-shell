// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waylanddockhelper.h"
#include "constants.h"
#include "dockpanel.h"

namespace dock {
WaylandDockHelper::WaylandDockHelper(DockPanel *panel)
    : DockHelper(panel)
    , m_panel(panel)
{
    m_wallpaperColorManager.reset(new WallpaperColorManager(this));

    connect(m_panel, &DockPanel::rootObjectChanged, this, [this]() {
        m_wallpaperColorManager->watchScreen(dockScreenName());
    });

    connect(m_panel, &DockPanel::dockScreenChanged, this, [this]() {
        m_wallpaperColorManager->watchScreen(dockScreenName());
    });

    connect(m_wallpaperColorManager.get(), &WallpaperColorManager::activeChanged, this, [this]() {
        if (m_panel->rootObject() != nullptr) {
            m_wallpaperColorManager->watchScreen(dockScreenName());
        }
    });

    if (m_panel->rootObject() != nullptr) {
        m_wallpaperColorManager->watchScreen(dockScreenName());
    }
}

void WaylandDockHelper::updateDockTriggerArea()
{
    
}

HideState WaylandDockHelper::hideState()
{
    return Show;
}

QString WaylandDockHelper::dockScreenName()
{
    if (m_panel->dockScreen())
        return m_panel->dockScreen()->name();

    return {};
}

void WaylandDockHelper::setDockColorTheme(const ColorTheme &theme)
{
    m_panel->setColorTheme(theme);
}

WallpaperColorManager::WallpaperColorManager(WaylandDockHelper *helper)
    : QWaylandClientExtensionTemplate<WallpaperColorManager>(treeland_wallpaper_color_manager_v1_interface.version)
    , m_helper(helper)
{
}

void WallpaperColorManager::treeland_wallpaper_color_manager_v1_output_color(const QString &output, uint32_t isDark)
{
    if (output == m_helper->dockScreenName()) {
        m_helper->setDockColorTheme(isDark ? Dark : Light);
    }
}

void WallpaperColorManager::watchScreen(const QString &screeName)
{
    if (isActive() && !screeName.isEmpty()) {
        watch(screeName);
    }
}
}
