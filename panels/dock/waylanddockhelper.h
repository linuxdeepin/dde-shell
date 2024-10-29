// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "constants.h"
#include "dockhelper.h"
#include "dockpanel.h"

#include "qwayland-treeland-dde-shell-v1.h"
#include "qwayland-treeland-wallpaper-color-v1.h"
#include <QWidget>

#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace dock {
class WallpaperColorManager;
class TreeLandDockTriggerArea;
class WaylandDockHelper : public DockHelper
{
    Q_OBJECT

public:
    WaylandDockHelper(DockPanel *panel);

    void setDockColorTheme(const ColorTheme &theme);
    QString dockScreenName();

protected:
    [[nodiscard]] virtual DockWakeUpArea *createArea(QScreen *screen) override;

public Q_SLOTS:
    void updateDockTriggerArea() override;

private:
    DockPanel *m_panel;
    QScopedPointer<WallpaperColorManager> m_wallpaperColorManager;
};

class WallpaperColorManager : public QWaylandClientExtensionTemplate<WallpaperColorManager>, public QtWayland::treeland_wallpaper_color_manager_v1
{
    Q_OBJECT
public:
    explicit WallpaperColorManager(WaylandDockHelper *helper);

    void watchScreen(const QString &screeName);
    void treeland_wallpaper_color_manager_v1_output_color(const QString &output, uint32_t isDark) override;

Q_SIGNALS:
    void wallpaperColorChanged(uint32_t isDark);

private:
    WaylandDockHelper *m_helper;
};

class TreeLandDDEShellManager : public QWaylandClientExtensionTemplate<TreeLandDDEShellManager>, public QtWayland::treeland_dde_shell_manager_v1
{
    Q_OBJECT

public:
    explicit TreeLandDDEShellManager();
};

class TreeLandWindowOverlapChecker : public QWaylandClientExtensionTemplate<TreeLandWindowOverlapChecker>, public QtWayland::treeland_window_overlap_checker
{
    Q_OBJECT

public:
    TreeLandWindowOverlapChecker(QtWaylandClient::QWaylandWindow *window, struct ::treeland_window_overlap_checker *);
    ~TreeLandWindowOverlapChecker();

protected:
    void treeland_window_overlap_checker_enter() override;
    void treeland_window_overlap_checker_leave() override;
};

class TreeLandDockWakeUpArea : public QWidget, public DockWakeUpArea
{
    Q_OBJECT
public:
    explicit TreeLandDockWakeUpArea(QScreen *screen, WaylandDockHelper *helper, DockPanel *panel);

public:
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
