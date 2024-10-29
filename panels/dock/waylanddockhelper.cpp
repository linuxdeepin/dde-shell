// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waylanddockhelper.h"
#include "constants.h"
#include "dockhelper.h"
#include "dockpanel.h"
#include "layershell/dlayershellwindow.h"
#include "wayland-treeland-dde-shell-v1-client-protocol.h"

#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandsurface_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace dock {
WaylandDockHelper::WaylandDockHelper(DockPanel *panel)
    : DockHelper(panel)
    , m_panel(panel)
{
    m_wallpaperColorManager.reset(new WallpaperColorManager(this));

    connect(m_panel, &DockPanel::rootObjectChanged, this, [this]() {
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

DockWakeUpArea *WaylandDockHelper::createArea(QScreen *screen)
{
    return new TreeLandDockWakeUpArea(screen, this, m_panel);
}

void WaylandDockHelper::updateDockTriggerArea()
{
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

TreeLandDDEShellManager::TreeLandDDEShellManager()
    : QWaylandClientExtensionTemplate<TreeLandDDEShellManager>(treeland_dde_shell_manager_v1_interface.version)
{
}

TreeLandWindowOverlapChecker::TreeLandWindowOverlapChecker(QtWaylandClient::QWaylandWindow *window, struct ::treeland_window_overlap_checker *checker)
    : QWaylandClientExtensionTemplate<TreeLandWindowOverlapChecker>(treeland_dde_shell_manager_v1_interface.version)
{
    init(checker);
    auto waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen *>(window->window()->screen()->handle());
    update(100, 100, anchor_right | anchor_left | anchor_bottom, waylandScreen->output());
}

void TreeLandWindowOverlapChecker::treeland_window_overlap_checker_enter()
{
}

void TreeLandWindowOverlapChecker::treeland_window_overlap_checker_leave()
{
}

TreeLandDockWakeUpArea::TreeLandDockWakeUpArea(QScreen *screen, WaylandDockHelper *helper, DockPanel *panel)
    : QWidget()
    , DockWakeUpArea(screen, helper)
{
    winId();
    window()->setScreen(screen);
    window()->resize(QSize(15, 15));

    auto window = ds::DLayerShellWindow::get(windowHandle());
    window->setLayer(ds::DLayerShellWindow::LayerOverlay);
    window->setAnchors({ds::DLayerShellWindow::AnchorBottom | ds::DLayerShellWindow::AnchorLeft | ds::DLayerShellWindow::AnchorRight});
    window->setExclusiveZone(-1);
}

void TreeLandDockWakeUpArea::open()
{
    show();
}

void TreeLandDockWakeUpArea::close()
{
    hide();
}

void TreeLandDockWakeUpArea::updateDockWakeArea(Position pos)
{
    m_pos = pos;
    ds::DLayerShellWindow::Anchors anchors = {0 | ds::DLayerShellWindow::AnchorNone};
    switch (pos) {
    case Top: {
        anchors = {ds::DLayerShellWindow::AnchorLeft | ds::DLayerShellWindow::AnchorRight | ds::DLayerShellWindow::AnchorTop};
        break;
    }
    case Right: {
        anchors = {ds::DLayerShellWindow::AnchorRight | ds::DLayerShellWindow::AnchorTop | ds::DLayerShellWindow::AnchorBottom};
        break;
    }
    case Bottom: {
        anchors = {ds::DLayerShellWindow::AnchorLeft | ds::DLayerShellWindow::AnchorRight | ds::DLayerShellWindow::AnchorBottom};
        break;
    }
    case Left: {
        anchors = {ds::DLayerShellWindow::AnchorLeft | ds::DLayerShellWindow::AnchorTop | ds::DLayerShellWindow::AnchorBottom};
        break;
    }
    }

    window()->resize(QSize(15, 15));
    auto window = ds::DLayerShellWindow::get(windowHandle());
    window->setAnchors(anchors);
}

TreeLandWindowOverlapChecker::~TreeLandWindowOverlapChecker()
{
    destroy();
}

void TreeLandDockWakeUpArea::enterEvent(QEnterEvent *event)
{
    m_helper->enterScreen(screen());
}

void TreeLandDockWakeUpArea::leaveEvent(QEvent *event)
{
    m_helper->leaveScreen();
}

void TreeLandDockWakeUpArea::resizeEvent(QResizeEvent *event)
{
    auto size = event->size();
    if (m_pos == Left || m_pos == Right) {
        size.setHeight(m_screen->size().height());
        size.setWidth(15);
    } else {
        size.setHeight(15);
        size.setWidth(m_screen->size().width());
    }

    window()->resize(size);
}
}
