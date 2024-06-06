// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include "dsglobal.h"
#include "constants.h"

#include <QDBusContext>

namespace dock {
class DockHelper;
const QStringList pluginDirs = {
    "/usr/lib/dde-dock/plugins/",
    "/usr/lib/dde-dock/plugins/quick-trays/",
    "/usr/lib/dde-dock/plugins/system-trays/"
};

class DockPanel : public DS_NAMESPACE::DPanel, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry FINAL)

    Q_PROPERTY(QRect frontendWindowRect READ frontendWindowRect NOTIFY frontendWindowRectChanged FINAL)
    Q_PROPERTY(bool compositorReady READ compositorReady WRITE setCompositorReady NOTIFY compositorReadyChanged FINAL)

    Q_PROPERTY(HideState hideState READ hideState NOTIFY hideStateChanged FINAL)
    Q_PROPERTY(ColorTheme colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged FINAL)

    Q_PROPERTY(uint dockSize READ dockSize WRITE setDockSize NOTIFY dockSizeChanged FINAL)
    Q_PROPERTY(HideMode hideMode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(ItemAlignment itemAlignment READ itemAlignment WRITE setItemAlignment NOTIFY itemAlignmentChanged FINAL)
    Q_PROPERTY(IndicatorStyle indicatorStyle READ indicatorStyle WRITE setIndicatorStyle NOTIFY indicatorStyleChanged FINAL)

    Q_PROPERTY(bool debugMode READ debugMode FINAL CONSTANT)

public:
    explicit DockPanel(QObject *parent = nullptr);

    bool load() override;
    bool init() override;

    void ReloadPlugins();
    void callShow();

    QRect geometry();
    QRect frontendWindowRect();

    HideState hideState();

    ColorTheme colorTheme();
    void setColorTheme(const ColorTheme& theme);

    uint dockSize();
    void setDockSize(const uint& size);

    HideMode hideMode();
    void setHideMode(const HideMode& mode);

    Position position();
    void setPosition(const Position& position);

    ItemAlignment itemAlignment();
    void setItemAlignment(const ItemAlignment& alignment);

    IndicatorStyle indicatorStyle();
    void setIndicatorStyle(const IndicatorStyle& style);

    bool compositorReady();
    void setCompositorReady(bool ready);

    bool debugMode() const;

    Q_INVOKABLE void openDockSettings() const;
    Q_INVOKABLE void setMouseGrabEnabled(QQuickItem *item, bool enabled);

private Q_SLOTS:
    void onWindowGeometryChanged();
    void loadDockPlugins();
    void launcherVisibleChanged(bool visible);

Q_SIGNALS:
    void geometryChanged(QRect geometry);
    void frontendWindowRectChanged(QRect frontendWindowRect);
    void hideStateChanged(HideState state); // not emitted
    void colorThemeChanged(ColorTheme theme);
    void compositorReadyChanged();

    void dockSizeChanged(uint size);
    void hideModeChanged(HideMode mode);
    void positionChanged(Position position);
    void itemAlignmentChanged(ItemAlignment alignment);
    void indicatorStyleChanged(IndicatorStyle style);

private:
    ColorTheme m_theme;
    HideState m_hideState;
    DockHelper* m_helper;
    bool m_compositorReady;
    bool m_launcherShown;
};

}
