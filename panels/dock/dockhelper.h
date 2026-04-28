// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "constants.h"
#include "dockpanel.h"

#include <QObject>
#include <QPointer>

class QTimer;

namespace dock {
class DockWakeUpArea;
class DockHelper : public QObject
{
    Q_OBJECT

public:
    DockHelper(DockPanel *parent);
    bool eventFilter(QObject *watched, QEvent *event) override;

    void enterScreen(QScreen *screen);
    void leaveScreen();

Q_SIGNALS:
    void isWindowOverlapChanged(bool overlap);
    void currentActiveWindowFullscreenChanged(bool fullscreen);

protected:
    DockPanel *parent();
    [[nodiscard]] virtual DockWakeUpArea *createArea(QScreen *screen) = 0;
    virtual void destroyArea(DockWakeUpArea *area) = 0;

    virtual bool currentActiveWindowFullscreened() = 0;
    virtual bool isWindowOverlap() = 0;

private:
    bool wakeUpAreaNeedShowOnThisScreen(QScreen *screen);

    void updateAllDockWakeArea();
    void updatePanelMouseState();
    void updateCursorPosition(QEvent *event);

public Q_SLOTS:
    void checkNeedHideOrNot();
    void checkNeedShowOrNot();

private:
    void initAreas();

private:
    QHash<QScreen *, DockWakeUpArea *> m_areas;
    QHash<QWindow *, bool> m_enters;
    QHash<QWindow *, bool> m_transientChildShows;
    QTimer *m_hideTimer;
    QTimer *m_showTimer;
    QTimer *m_cursorMonitorTimer;
    QTimer *m_edgeWakeHoldTimer;
    bool m_edgeWakeLatched = false;
    QPointer<QScreen> m_edgeWakeScreen;
};

class DockWakeUpArea
{
public:
    QScreen *screen();
    virtual void open() = 0;
    virtual void close() = 0;

protected:
    explicit DockWakeUpArea(QScreen *screen, DockHelper *helper);
    virtual ~DockWakeUpArea() = default;
    virtual void updateDockWakeArea(Position pos) = 0;

protected:
    friend class DockHelper;
    QScreen *m_screen;
    DockHelper *m_helper;
};
}
