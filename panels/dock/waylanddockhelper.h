// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "dockpanel.h"

#include "qwayland-treeland-wallpaper-color-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>

namespace dock {
class WallpaperColorManager;
class WaylandDockHelper : public DockHelper
{
    Q_OBJECT

public:
    WaylandDockHelper(DockPanel* panel);
    HideState hideState() override;

    void setDockColorTheme(const ColorTheme &theme);
    QString dockScreenName();

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
}
