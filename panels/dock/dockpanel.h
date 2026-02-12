// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include "dsglobal.h"
#include "constants.h"

#include <QDBusContext>
#include <QPointer>

namespace dock {
class DockHelper;
class LoadTrayPlugins;

class DockPanel : public DS_NAMESPACE::DPanel, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry FINAL)

    Q_PROPERTY(QRect frontendWindowRect READ frontendWindowRect NOTIFY frontendWindowRectChanged FINAL)
    Q_PROPERTY(bool compositorReady READ compositorReady WRITE setCompositorReady NOTIFY compositorReadyChanged FINAL)

    Q_PROPERTY(HideState hideState READ hideState WRITE setHideState NOTIFY hideStateChanged FINAL)
    Q_PROPERTY(ColorTheme colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged FINAL)

    Q_PROPERTY(uint dockSize READ dockSize WRITE setDockSize NOTIFY dockSizeChanged FINAL)
    Q_PROPERTY(HideMode hideMode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(ItemAlignment itemAlignment READ itemAlignment WRITE setItemAlignment NOTIFY itemAlignmentChanged FINAL)
    Q_PROPERTY(IndicatorStyle indicatorStyle READ indicatorStyle WRITE setIndicatorStyle NOTIFY indicatorStyleChanged FINAL)
    Q_PROPERTY(bool showInPrimary READ showInPrimary WRITE setShowInPrimary NOTIFY showInPrimaryChanged FINAL)
    Q_PROPERTY(QString screenName READ screenName NOTIFY screenNameChanged FINAL)
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged FINAL)
    Q_PROPERTY(bool isResizing READ isResizing WRITE setIsResizing NOTIFY isResizingChanged FINAL)

    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged FINAL)

    Q_PROPERTY(bool debugMode READ debugMode FINAL CONSTANT)

    Q_PROPERTY(bool contextDragging READ contextDragging WRITE setContextDragging NOTIFY contextDraggingChanged FINAL)

public:
    explicit DockPanel(QObject *parent = nullptr);

    bool load() override;
    bool init() override;

    void ReloadPlugins();
    void callShow();

    QRect geometry();
    QRect frontendWindowRect();
    void setFrontendWindowRect(int transformOffsetX, int transformOffsetY);

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

    Q_INVOKABLE void notifyDockPositionChanged(int offsetX, int offsetY);

    bool showInPrimary() const;
    void setShowInPrimary(bool newShowInPrimary);

    bool locked() const;
    void setLocked(bool newLocked);

    void setHideState(HideState newHideState);
    QScreen* dockScreen();
    void setDockScreen(QScreen *screen);
    QString screenName() const;

    qreal devicePixelRatio() const;

    bool contextDragging() const;
    void setContextDragging(bool newContextDragging);

    bool isResizing() const;
    void setIsResizing(bool resizing);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void onWindowGeometryChanged();
    void launcherVisibleChanged(bool visible);
    void updateDockScreen();

Q_SIGNALS:
    void geometryChanged(QRect geometry);
    void frontendWindowRectChanged(QRect frontendWindowRect);
    void hideStateChanged(HideState state);
    void colorThemeChanged(ColorTheme theme);
    void compositorReadyChanged();

    void dockSizeChanged(uint size);
    void hideModeChanged(HideMode mode);
    void beforePositionChanged(Position beforePosition);
    void positionChanged(Position position);
    void itemAlignmentChanged(ItemAlignment alignment);
    void indicatorStyleChanged(IndicatorStyle style);
    void showInPrimaryChanged(bool showInPrimary);
    void dockScreenChanged(QScreen *screen);
    void screenNameChanged();
    void requestClosePopup();
    void leftEdgeClicked(const QString &minOrder);
    void devicePixelRatioChanged(qreal ratio);
    void lockedChanged(bool locked);

    void contextDraggingChanged();
    void isResizingChanged(bool isResizing);

private:
    ColorTheme m_theme;
    HideState m_hideState;
    DockHelper* m_helper;
    QPointer<QScreen> m_dockScreen;
    LoadTrayPlugins *m_loadTrayPlugins;
    bool m_compositorReady;
    bool m_launcherShown;
    bool m_contextDragging;
    bool m_isResizing;
    QRect m_frontendWindowRect;
};

}
