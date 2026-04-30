// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockhelper.h"
#include "constants.h"
#include "dockpanel.h"

#include <QCursor>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QWindow>

namespace dock
{
namespace {

QWindow *topTransientParent(QWindow *window)
{
    QWindow *topLevelWindow = window;
    while (topLevelWindow && topLevelWindow->transientParent()) {
        topLevelWindow = topLevelWindow->transientParent();
    }

    return topLevelWindow;
}

bool isDockRelatedWindow(QWindow *window, QWindow *dockWindow)
{
    if (!window || !dockWindow) {
        return false;
    }

    if (window == dockWindow) {
        return true;
    }

    return topTransientParent(window) == dockWindow;
}

QRect expandedGeometry(const QRect &geometry, int margin)
{
    return geometry.adjusted(-margin, -margin, margin, margin);
}

QRect dockMouseTrackingGeometry(const QRect &geometry, const QRect &screenGeometry, Position position, int margin)
{
    QRect trackingGeometry = expandedGeometry(geometry, margin);

    const int leftGap = qMax(0, geometry.left() - screenGeometry.left());
    const int topGap = qMax(0, geometry.top() - screenGeometry.top());
    const int rightGap = qMax(0, screenGeometry.right() - geometry.right());
    const int bottomGap = qMax(0, screenGeometry.bottom() - geometry.bottom());

    switch (position) {
    case Bottom:
        trackingGeometry.adjust(0, 0, 0, qMax(0, bottomGap - margin));
        break;
    case Top:
        trackingGeometry.adjust(0, -qMax(0, topGap - margin), 0, 0);
        break;
    case Left:
        trackingGeometry.adjust(-qMax(0, leftGap - margin), 0, 0, 0);
        break;
    case Right:
        trackingGeometry.adjust(0, 0, qMax(0, rightGap - margin), 0);
        break;
    }

    return trackingGeometry;
}

bool isCursorOnDockWakeEdge(const QPoint &globalCursorPos, const QRect &screenGeometry, Position position)
{
    if (!screenGeometry.isValid()) {
        return false;
    }

    constexpr int edgeThreshold = 2;
    if (!screenGeometry.adjusted(-edgeThreshold, -edgeThreshold, edgeThreshold, edgeThreshold).contains(globalCursorPos)) {
        return false;
    }

    switch (position) {
    case Bottom:
        return globalCursorPos.y() >= screenGeometry.bottom() - edgeThreshold;
    case Top:
        return globalCursorPos.y() <= screenGeometry.top() + edgeThreshold;
    case Left:
        return globalCursorPos.x() <= screenGeometry.left() + edgeThreshold;
    case Right:
        return globalCursorPos.x() >= screenGeometry.right() - edgeThreshold;
    }

    return false;
}

bool shouldUseCursorEdgeWakeFallback(DockPanel *panel)
{
    if (!panel) {
        return false;
    }

    // Fashion mode uses its own wake-up surface. Keeping the generic edge fallback
    // enabled there causes premature wake-ups and repeated hide/show loops while
    // the cursor is still parked near the screen edge.
    return panel->viewMode() != FashionMode;
}

}

DockHelper::DockHelper(DockPanel *parent)
    : QObject(parent)
    , m_hideTimer(new QTimer(this))
    , m_showTimer(new QTimer(this))
    , m_cursorMonitorTimer(new QTimer(this))
    , m_edgeWakeHoldTimer(new QTimer(this))
{
    m_hideTimer->setInterval(400);
    m_showTimer->setInterval(120);
    m_cursorMonitorTimer->setInterval(16);
    m_edgeWakeHoldTimer->setInterval(420);
    m_hideTimer->setSingleShot(true);
    m_showTimer->setSingleShot(true);
    m_edgeWakeHoldTimer->setSingleShot(true);

    qApp->installEventFilter(this);
    QMetaObject::invokeMethod(this, &DockHelper::initAreas, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &DockHelper::checkNeedShowOrNot, Qt::QueuedConnection);

    connect(parent, &DockPanel::rootObjectChanged, this, &DockHelper::initAreas);
    connect(parent, &DockPanel::showInPrimaryChanged, this, &DockHelper::updateAllDockWakeArea);
    connect(parent, &DockPanel::hideStateChanged, this, &DockHelper::updateAllDockWakeArea);
    connect(parent, &DockPanel::viewModeChanged, this, [this]() {
        updateAllDockWakeArea();
        updatePanelMouseState();
    });
    connect(parent, &DockPanel::hideModeChanged, m_hideTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(parent, &DockPanel::hideModeChanged, m_showTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(parent, &DockPanel::positionChanged, this, [this](Position pos) {
        std::for_each(m_areas.begin(), m_areas.end(), [pos](const auto &area) {
            if (!area)
                return;
            area->updateDockWakeArea(pos);
        });
    });

    connect(m_hideTimer, &QTimer::timeout, this, &DockHelper::checkNeedHideOrNot);
    connect(m_showTimer, &QTimer::timeout, this, &DockHelper::checkNeedShowOrNot);
    connect(m_cursorMonitorTimer, &QTimer::timeout, this, &DockHelper::updatePanelMouseState);
    connect(m_edgeWakeHoldTimer, &QTimer::timeout, this, &DockHelper::checkNeedHideOrNot);
    m_cursorMonitorTimer->start();

    connect(this, &DockHelper::isWindowOverlapChanged, this, [this](bool overlap) {
        if (overlap) {
            if (m_showTimer->isActive()) {
                m_showTimer->stop();
            }
            m_hideTimer->start();
        } else {
            if (m_hideTimer->isActive()) {
                m_hideTimer->stop();
            }
            m_showTimer->start();
        }
    });
    connect(this, &DockHelper::currentActiveWindowFullscreenChanged, this, [this] (bool isFullscreen) {
        if (m_hideTimer->isActive()) {
            m_hideTimer->stop();
        }
        if (m_showTimer->isActive()) {
            m_showTimer->stop();
        }
        if (isFullscreen) {
            m_hideTimer->start();
        } else {
            m_showTimer->start();
        }
    });
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

    // skip tooltip windows
    if (window->flags().testFlags(Qt::ToolTip)) {
        if (!window->property("isDockPreview").toBool()) {
            return false;
        }
    }

    auto topTransientParent = window;
    while (topTransientParent->transientParent()) {
        topTransientParent = topTransientParent->transientParent();
    }

    // not dock panel or dock popup has a enter event
    if (window != parent()->rootObject() && topTransientParent != parent()->rootObject()) {
        return false;
    }

    switch (event->type()) {
    case QEvent::Enter: {
        m_enters.insert(window, true);
        updateCursorPosition(event);
        if (m_edgeWakeHoldTimer->isActive()) {
            m_edgeWakeHoldTimer->stop();
        }
        if (m_hideTimer->isActive()) {
            m_hideTimer->stop();
        }

        m_showTimer->start();
        break;
    }
    case QEvent::MouseMove:
    case QEvent::HoverMove: {
        updateCursorPosition(event);
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
    case QEvent::Show: {
        // if the dock's subwindow is show, the dock is not hidden
        if (window->transientParent() != NULL && 
            topTransientParent == parent()->rootObject()) {
            m_transientChildShows.insert(window, true);
            if (m_hideTimer->isActive()) {
                m_hideTimer->stop();
            }
        }
        break;
    }
    case QEvent::Hide: {
        m_enters.remove(window);
        if (window->transientParent() != NULL && 
            topTransientParent == parent()->rootObject()) {
            m_transientChildShows.remove(window);
            m_hideTimer->start();
        }
        break;
    }
    default: {
    }
    }

    updatePanelMouseState();
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
    if (!screen) {
        return;
    }

    if (m_edgeWakeLatched && m_edgeWakeScreen == screen) {
        return;
    }

    m_edgeWakeLatched = true;
    m_edgeWakeScreen = screen;

    if (m_hideTimer->isActive()) {
        m_hideTimer->stop();
    }
    if (m_showTimer->isActive()) {
        m_showTimer->stop();
    }

    auto nowScreen = parent()->dockScreen();

    if (nowScreen == screen) {
        m_edgeWakeHoldTimer->start();
        parent()->setHideState(Show);
        return;
    }

    // Do not switch screen if any popup/transient child window is showing
    for (auto show : m_transientChildShows) {
        if (show) {
            m_edgeWakeHoldTimer->start();
            parent()->setHideState(Show);
            return;
        }
    }

    QTimer::singleShot(200, [this, screen]() {
        if (!screen) {
            return;
        }

        m_edgeWakeHoldTimer->start();
        parent()->setDockScreen(screen);
        parent()->setHideState(Show);
        updateAllDockWakeArea();
    });
}

void DockHelper::leaveScreen()
{
    if (m_edgeWakeHoldTimer->isActive()) {
        m_edgeWakeHoldTimer->stop();
    }
    m_hideTimer->start();
}

void DockHelper::updateAllDockWakeArea()
{
    for (auto screen : m_areas.keys()) {
        auto area = m_areas.value(screen);
        if (nullptr == area)
            continue;

        if (wakeUpAreaNeedShowOnThisScreen(screen)) {
            area->open();
            area->updateDockWakeArea(parent()->position());
        } else {
            area->close();
        }
    }
}

void DockHelper::updatePanelMouseState()
{
    auto *window = parent()->window();
    const QPoint globalCursorPos = QCursor::pos();
    QScreen *cursorScreen = QGuiApplication::screenAt(globalCursorPos);
    if (!cursorScreen && window) {
        cursorScreen = window->screen();
    }

    const bool cursorOnWakeEdge = cursorScreen
        && isCursorOnDockWakeEdge(globalCursorPos, cursorScreen->geometry(), parent()->position());

    if (!cursorScreen
        || cursorScreen != m_edgeWakeScreen
        || !cursorOnWakeEdge) {
        m_edgeWakeLatched = false;
        m_edgeWakeScreen.clear();
    }

    if (shouldUseCursorEdgeWakeFallback(parent())
        && parent()->hideState() == Hide
        && cursorScreen
        && (!parent()->showInPrimary() || cursorScreen == qApp->primaryScreen())
        && cursorOnWakeEdge) {
        enterScreen(cursorScreen);
    }

    if (!window || !window->isVisible()) {
        parent()->setContainsMouse(false);
        return;
    }

    const int geometryMargin = qMax(6, qRound(parent()->dockSize() * 0.14));
    const QRect screenGeometry = window->screen() ? window->screen()->geometry() : QRect();
    const Position panelPosition = parent()->position();
    bool containsMouse = dockMouseTrackingGeometry(window->geometry(), screenGeometry, panelPosition, geometryMargin).contains(globalCursorPos);

    if (!containsMouse) {
        const auto topLevelWindows = QGuiApplication::topLevelWindows();
        for (QWindow *candidateWindow : topLevelWindows) {
            if (!candidateWindow || !candidateWindow->isVisible() || !isDockRelatedWindow(candidateWindow, window)) {
                continue;
            }

            const QRect candidateScreenGeometry = candidateWindow->screen() ? candidateWindow->screen()->geometry() : screenGeometry;
            if (dockMouseTrackingGeometry(candidateWindow->geometry(), candidateScreenGeometry, panelPosition, geometryMargin).contains(globalCursorPos)) {
                containsMouse = true;
                break;
            }
        }
    }

    parent()->setContainsMouse(containsMouse);
    if (containsMouse) {
        m_edgeWakeLatched = false;
        m_edgeWakeScreen.clear();
        if (m_edgeWakeHoldTimer->isActive()) {
            m_edgeWakeHoldTimer->stop();
        }
        parent()->setCursorPosition(globalCursorPos - window->position());
    }
}

void DockHelper::updateCursorPosition(QEvent *event)
{
    if (!parent()->window() || !event) {
        return;
    }

    QPointF globalPosition;
    bool valid = false;
    switch (event->type()) {
    case QEvent::Enter: {
        auto *enterEvent = static_cast<QEnterEvent *>(event);
        globalPosition = enterEvent->globalPosition();
        valid = true;
        break;
    }
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        globalPosition = mouseEvent->globalPosition();
        valid = true;
        break;
    }
    case QEvent::HoverMove: {
        auto *hoverEvent = static_cast<QHoverEvent *>(event);
        globalPosition = hoverEvent->globalPosition();
        valid = true;
        break;
    }
    default:
        break;
    }

    if (!valid) {
        return;
    }

    parent()->setCursorPosition(globalPosition - parent()->window()->position());
}

void DockHelper::checkNeedHideOrNot()
{
    if (m_edgeWakeHoldTimer->isActive()) {
        return;
    }

    bool needHide;
    switch (parent()->hideMode()) {
    case KeepShowing: {
        // KeepShow. current activeWindow is fullscreend.
        needHide = currentActiveWindowFullscreened();
        break;
    }
    case SmartHide: {
        // SmartHide. window overlap.
        needHide = isWindowOverlap();
        break;
    }
    case KeepHidden: {
        // only any enter
        needHide = true;
        break;
    }
    }

    needHide &= !parent()->contextDragging();
    needHide &= !parent()->containsMouse();

    // any enter will not make hide
    for (auto enter : m_enters) {
        needHide &= !enter;
    }

    for (auto show : m_transientChildShows) {
        needHide &= !show;
    }    

    if (needHide)
        parent()->setHideState(Hide);
}

void DockHelper::checkNeedShowOrNot()
{
    bool needShow;
    switch (parent()->hideMode()) {
    case KeepShowing: {
        // KeepShow. currentWindow is not fullscreened.
        needShow = !currentActiveWindowFullscreened();
        break;
    }
    case SmartHide: {
        // SmartHide. no window overlap.
        needShow = !isWindowOverlap();
        break;
    }
    case KeepHidden: {
        // KeepHidden only any enter.
        needShow = false;
        break;
    }
    }

    for (auto enter : m_enters) {
        needShow |= enter;
    }

    if (needShow)
        parent()->setHideState(Show);
}

DockPanel* DockHelper::parent()
{
    return static_cast<DockPanel *>(QObject::parent());
}

void DockHelper::initAreas()
{
    // clear old area
    for (auto area : m_areas) {
        if (!area) {
            continue;
        }

        destroyArea(area);
    }

    m_areas.clear();
    qApp->disconnect(this);

    if (!parent()->rootObject())
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
}

DockWakeUpArea::DockWakeUpArea(QScreen *screen, DockHelper *helper)
    : m_screen(screen)
    , m_helper(helper)
{
}

QScreen *DockWakeUpArea::screen()
{
    return m_screen;
}
}
