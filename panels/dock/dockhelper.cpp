// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "constants.h"
#include "dockpanel.h"

#include <QGuiApplication>

namespace dock
{
DockHelper::DockHelper(DockPanel *parent)
    : QObject(parent)
    , m_hideTimer(new QTimer(this))
    , m_showTimer(new QTimer(this))
{
    m_hideTimer->setInterval(400);
    m_showTimer->setInterval(400);
    m_hideTimer->setSingleShot(true);
    m_showTimer->setSingleShot(true);

    connect(m_hideTimer, &QTimer::timeout, this, [this, parent]() {
        if (parent->hideMode() == KeepShowing)
            return;

        for (auto enter : m_enters) {
            if (enter)
                return;
        }

        parent->setHideState(Hide);
    });

    connect(m_showTimer, &QTimer::timeout, this, [this, parent]() {
        bool res = false;
        for (auto enter : m_enters) {
            res |= enter;
        }

        if (res)
            parent->setHideState(Show);
    });

    auto initAreas = [this, parent]() {
        // clear old area
        for (auto area : m_areas) {
            area->close();
            delete area;
        }

        m_areas.clear();
        qApp->disconnect(this);

        if (!parent->rootObject())
            return;

        // init areas
        for (auto screen : qApp->screens()) {
            m_areas.insert(screen, createArea(screen));
        }

        connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
            if (m_areas.contains(screen))
                return;
            m_areas.insert(screen, createArea(screen));
        });

        connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
            if (!m_areas.contains(screen))
                return;

            destroyArea(m_areas.value(screen));
            m_areas.remove(screen);
        });

        // init state
        updateAllDockWakeArea();
    };

    connect(parent, &DockPanel::rootObjectChanged, this, initAreas);
    connect(parent, &DockPanel::showInPrimaryChanged, this, &DockHelper::updateAllDockWakeArea);
    connect(parent, &DockPanel::hideStateChanged, this, &DockHelper::updateAllDockWakeArea);
    connect(parent, &DockPanel::positionChanged, this, [this](Position pos) {
        for (auto area : m_areas) {
            if (!area)
                continue;

            area->updateDockWakeArea(pos);
        }
    });

    qApp->installEventFilter(this);
    QMetaObject::invokeMethod(this, initAreas);
}

bool DockHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (!watched->isWindowType()) {
        return false;
    }

    auto window = static_cast<QWindow *>(watched);
    if (!window) {
        return false;
    }

    switch (event->type()) {
    case QEvent::Enter: {
        m_enters.insert(window, true);
        if (m_hideTimer->isActive()) {
            m_hideTimer->stop();
        }

        m_showTimer->start();
        break;
    }
    case QEvent::Leave: {
        m_enters.insert(window, false);
        if (m_showTimer->isActive()) {
            m_showTimer->stop();
        }

        m_hideTimer->start();
        break;
    }

    case QEvent::Hide: {
        m_enters.remove(window);
        break;
    }
    default: {
    }
    }

    return false;
}

bool DockHelper::wakeUpAreaNeedShowOnThisScreen(QScreen *screen)
{
    auto isDockAllowedShownOnThisScreen = [this, screen]() -> bool {
        return (parent()->showInPrimary() && screen == qApp->primaryScreen()) || !parent()->showInPrimary();
    };

    auto isDockShownThisScreen = [this, screen]() -> bool {
        return parent()->dockScreen() == screen && parent()->hideState() == Show;
    };

    return (isDockAllowedShownOnThisScreen() && !isDockShownThisScreen());
}

void DockHelper::enterScreen(QScreen *screen)
{
    auto nowScreen = parent()->dockScreen();

    if (nowScreen == screen) {
        parent()->setHideState(Show);
        return;
    }

    QTimer::singleShot(200, [this, screen]() {
        parent()->setDockScreen(screen);
        parent()->setHideState(Show);
        updateAllDockWakeArea();
    });
}

void DockHelper::leaveScreen()
{
    m_hideTimer->start();
}

DockWakeUpArea *DockHelper::createArea(QScreen *screen)
{
    return new DockWakeUpArea(screen, this);
}

void DockHelper::destroyArea(DockWakeUpArea *area)
{
    if (area) {
        area->close();
        delete area;
    }
}

void DockHelper::updateAllDockWakeArea()
{
    for (auto screen : m_areas.keys()) {
        auto area = m_areas.value(screen);
        if (nullptr == area)
            continue;

        area->updateDockWakeArea(parent()->position());
        if (wakeUpAreaNeedShowOnThisScreen(screen)) {
            area->open();
        } else {
            area->close();
        }
    }
}

DockPanel* DockHelper::parent()
{
    return static_cast<DockPanel *>(QObject::parent());
}

DockWakeUpArea::DockWakeUpArea(QScreen *screen, DockHelper *helper)
    : m_screen(screen)
    , m_helper(helper)
{
}

void DockWakeUpArea::open()
{
    qDebug() << "create wake up area at " << m_screen->name();
}

void DockWakeUpArea::close()
{
    qDebug() << "close wake up area at " << m_screen->name();
}

void DockWakeUpArea::updateDockWakeArea(Position pos)
{
    qDebug() << "update wake up area pos to " << pos;
}
}
