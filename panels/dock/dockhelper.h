// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "constants.h"
#include "dockpanel.h"

#include <QObject>

namespace dock {
class DockWakeUpArea;
class DockHelper : public QObject
{
    Q_OBJECT

public:
    [[nodiscard]] DockHelper* getHelper(DockPanel* parent);

    virtual HideState hideState()
    {
        return Show;
    }

    bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
    void enterScreen(QScreen *screen);
    void leaveScreen();

    virtual void updateDockTriggerArea() = 0;

Q_SIGNALS:
    void hideStateChanged();

protected:
    bool wakeUpAreaNeedShowOnThisScreen(QScreen *screen);
    [[nodiscard]] virtual DockWakeUpArea *createArea(QScreen *screen);
    virtual void destroyArea(DockWakeUpArea *area);

private:
    void updateAllDockWakeArea();

protected:
    DockHelper(DockPanel* parent);
    DockPanel* parent();

private:
    QHash<QScreen *, DockWakeUpArea *> m_areas;
    QHash<QWindow *, bool> m_enters;
    QTimer *m_hideTimer;
    QTimer *m_showTimer;
};

class DockWakeUpArea
{
public:
    virtual void open();
    virtual void close();

protected:
    explicit DockWakeUpArea(QScreen *screen, DockHelper *helper);
    virtual void updateDockWakeArea(Position pos);

protected:
    friend class DockHelper;
    QScreen *m_screen;
    DockHelper *m_helper;
};
}

