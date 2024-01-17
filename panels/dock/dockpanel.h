// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include "dsglobal.h"
#include "constants.h"

#include <QDBusContext>

DS_BEGIN_NAMESPACE
namespace dock {
const QStringList pluginDirs = {
    "/usr/lib/dde-dock/plugins/",
    "/usr/lib/dde-dock/plugins/quick-trays/",
    "/usr/lib/dde-dock/plugins/system-trays/"
};

class DockPanel : public DPanel, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry FINAL)

    Q_PROPERTY(QRect frontendWindowRect READ frontendWindowRect NOTIFY frontendWindowRectChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)

    Q_PROPERTY(HideState hideState READ hideState NOTIFY hideStateChanged FINAL)
    Q_PROPERTY(HideMode hideMode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode NOTIFY displayModeChanged FINAL)
    Q_PROPERTY(ColorTheme colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged FINAL)
    Q_PROPERTY(uint dockSize READ dockSize WRITE setDockSize NOTIFY dockSizeChanged FINAL)
    Q_PROPERTY(bool compositorReady READ compositorReady WRITE setCompositorReady NOTIFY compositorReadyChanged FINAL)
    Q_PROPERTY(bool debugMode READ debugMode)

public:
    explicit DockPanel(QObject *parent = nullptr);

    bool load() override;
    bool init() override;

    void ReloadPlugins();
    void callShow();

    QRect geometry();
    QRect frontendWindowRect();

    Position position();
    void setPosition(Position position);

    HideMode hideMode();
    void setHideMode(HideMode mode);

    DisplayMode displayMode();
    void setDisplayMode(DisplayMode mode);

    ColorTheme colorTheme();
    void setColorTheme(ColorTheme theme);

    HideState hideState();

    uint dockSize();
    void setDockSize(uint size);

    bool compositorReady();
    void setCompositorReady(bool ready);

    bool debugMode() const;

private Q_SLOTS:
    void onWindowGeometryChanged();
    void loadDockPlugins();

Q_SIGNALS:
    void frontendWindowRectChanged(QRect frontendWindowRect);
    void geometryChanged(QRect geometry);
    void positionChanged(Position position);
    void hideModeChanged(HideMode mode); // not emitted
    void hideStateChanged(HideState state); // not emitted
    void displayModeChanged(DisplayMode mode);
    void dockSizeChanged(uint size); // not emitted
    void colorThemeChanged(ColorTheme theme);
    void compositorReadyChanged();

private:
    ColorTheme m_theme;
    bool m_compositorReady;
};

}
DS_END_NAMESPACE
