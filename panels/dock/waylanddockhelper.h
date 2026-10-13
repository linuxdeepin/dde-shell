// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "constants.h"
#include "dockhelper.h"
#include "dockpanel.h"

#include "qwayland-treeland-wallpaper-color-v1.h"
#include "qwayland-treeland-xwindow-control-unstable-v1.h"
#include "qwayland-treeland-region-watch-unstable-v1.h"
#include <QWidget>

#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <qscopedpointer.h>

namespace dock {
class WallpaperColorManager;
class TreeLandDockTriggerArea;
class TreeLandXWindowControl;
class TreeLandRegionWatchManager;
class TreeLandRegionWatch;

class WaylandDockHelper : public DockHelper
{
    Q_OBJECT

public:
    WaylandDockHelper(DockPanel *panel);

    void setDockColorTheme(const ColorTheme &theme);
    QString dockScreenName();
    
    // Move XEmbed window relative to anchor window's surface
    // anchorWindow: the window containing the plugin item; falls back to dock panel if null
    // dx, dy: offset relative to anchor window top-left
    // Returns true if the request was sent (does NOT guarantee completion)
    bool moveXEmbedWindow(uint32_t wid, double dx, double dy, QQuickWindow *anchorWindow = nullptr) override;

Q_SIGNALS:
    void xembedWindowMoveResult(uint32_t wid, bool success);

protected:
    bool currentActiveWindowFullscreened() override;
    bool isWindowOverlap() override;
    [[nodiscard]] virtual DockWakeUpArea *createArea(QScreen *screen) override;
    void destroyArea(DockWakeUpArea *area) override;

protected Q_SLOTS:
    void setCurrentActiveWindowFullscreened(bool);

private:
    void updateRegionWatchPos();
    void initWallpaperColorManager();

private:
    friend class TreeLandRegionWatch;
    bool m_isWindowOverlap;
    bool m_isCurrentActiveWindowFullscreened;
    DockPanel *m_panel;
    QScopedPointer<WallpaperColorManager> m_wallpaperColorManager;
    QScopedPointer<TreeLandRegionWatch> m_regionWatch;
    QScopedPointer<TreeLandRegionWatchManager> m_regionWatchManager;
    QScopedPointer<TreeLandXWindowControl> m_xwindowControl;
    struct ::wl_surface *m_dockWlSurface = nullptr;  // Dock's wl_surface for XEmbed positioning
};

class WallpaperColorManager : public QWaylandClientExtensionTemplate<WallpaperColorManager>, public QtWayland::treeland_wallpaper_color_manager_v1
{
    Q_OBJECT
public:
    explicit WallpaperColorManager(WaylandDockHelper *helper);
    ~WallpaperColorManager() override;

    void watchScreen(const QString &screeName);
    void treeland_wallpaper_color_manager_v1_output_color(const QString &output, uint32_t isDark) override;

Q_SIGNALS:
    void wallpaperColorChanged(uint32_t isDark);

private:
    WaylandDockHelper *m_helper;
};

class TreeLandXWindowControl : public QWaylandClientExtensionTemplate<TreeLandXWindowControl>, public QtWayland::treeland_xwindow_control_v1
{
    Q_OBJECT

public:
    explicit TreeLandXWindowControl();

    ~TreeLandXWindowControl() override;

    struct ::wl_callback *setXWindowPositionRelative(uint32_t wid, struct ::wl_surface *anchor, double dx, double dy);
};

class TreeLandRegionWatchManager : public QWaylandClientExtensionTemplate<TreeLandRegionWatchManager>, public QtWayland::treeland_region_watch_manager_v1
{
    Q_OBJECT

public:
    explicit TreeLandRegionWatchManager();
    ~TreeLandRegionWatchManager() override;
};

class TreeLandRegionWatch : public QWaylandClientExtensionTemplate<TreeLandRegionWatch>, public QtWayland::treeland_region_watch_v1
{
    Q_OBJECT

public:
    TreeLandRegionWatch(WaylandDockHelper *helper, struct ::treeland_region_watch_v1 *);
    ~TreeLandRegionWatch();

protected:
    void treeland_region_watch_v1_enter() override;
    void treeland_region_watch_v1_leave() override;
    void treeland_region_watch_v1_output_removed() override;

private:
    WaylandDockHelper *m_helper;
};

class TreeLandDockWakeUpArea : public QWidget, public DockWakeUpArea
{
    Q_OBJECT
public:
    explicit TreeLandDockWakeUpArea(QScreen *screen, WaylandDockHelper *helper);

    void open() override;
    void close() override;

    void updateDockWakeArea(Position pos) override;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Position m_pos;
};
}
