// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "dlayershellwindow.h"
#include "x11dlayershellemulation.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMargins>
#include <QScreen>

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/shape.h>

DS_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(layershell, "org.deepin.dde.shell.layershell")

LayerShellEmulation::LayerShellEmulation(QWindow* window, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_dlayerShellWindow(DLayerShellWindow::get(m_window))
    , m_screen(m_dlayerShellWindow->screen())
{
    // 创建时锁存 output。客户端（如桌面）在创建 layer surface 之前已经
    // QWindow::setScreen()，此处取到的就是它声明的屏。此后 QWindow::screen()
    // 会随窗口几何被 Qt 自动推导，导致"窗口压在哪块屏上就属于哪块屏"的
    // 正反馈，所以不能再拿它当归属依据。
    qCInfo(layershell) << "layer surface bound to screen:" << (m_screen ? m_screen->name() : QStringLiteral("<null>"))
                       << "winId:" << m_window->winId();
    onLayerChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::layerChanged, this, &LayerShellEmulation::onLayerChanged);

    onPositionChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_dlayerShellWindow, &DLayerShellWindow::marginsChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_dlayerShellWindow, &DLayerShellWindow::geometryHintsChanged, this, &LayerShellEmulation::onPositionChanged);

    onExclusionZoneChanged();
    m_exclusionZoneChangedTimer.setSingleShot(true);
    m_exclusionZoneChangedTimer.setInterval(100);
    connect(&m_exclusionZoneChangedTimer, &QTimer::timeout, this, &LayerShellEmulation::onExclusionZoneChanged);
    connect(m_dlayerShellWindow, &DLayerShellWindow::anchorsChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_dlayerShellWindow, &DLayerShellWindow::exclusionZoneChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    // qml height or width may update later, need to update anchor postion and exclusion zone
    connect(m_window, &QWindow::widthChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_window, &QWindow::widthChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_window, &QWindow::heightChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_window, &QWindow::heightChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_window, &QWindow::xChanged, this, &LayerShellEmulation::onPositionChanged);
    connect(m_window, &QWindow::yChanged, this, &LayerShellEmulation::onPositionChanged);

    for (auto screen : qApp->screens()) {
        connect(screen, &QScreen::geometryChanged, this, &LayerShellEmulation::onPositionChanged);
        connect(screen, &QScreen::geometryChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    }
    connect(qApp, &QGuiApplication::screenAdded, this, [this] (const QScreen *newScreen) {
        connect(newScreen, &QScreen::geometryChanged, this, &LayerShellEmulation::onPositionChanged);
        connect(newScreen, &QScreen::geometryChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
        m_exclusionZoneChangedTimer.start();
    });
    connect(qApp, &QGuiApplication::primaryScreenChanged, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_window, &QWindow::screenChanged, this, [this](QScreen *nowScreen){
        Q_UNUSED(nowScreen)
        onPositionChanged();
        m_exclusionZoneChangedTimer.start();
    });

    // 只有显式重绑（DLayerShellWindow::setScreen）才允许改变归属屏，
    // 客户端靠它把已有 surface 搬到另一块屏（如 dock 切换主屏）。
    connect(m_dlayerShellWindow, &DLayerShellWindow::screenChanged, this, [this](QScreen *newScreen) {
        m_screen = newScreen ? newScreen : m_window->screen();
        qCInfo(layershell) << "layer surface rebound to screen:" << (m_screen ? m_screen->name() : QStringLiteral("<null>"));
        onPositionChanged();
        m_exclusionZoneChangedTimer.start();
    });

    // 锁存的 output 被拔掉时退回窗口当前所在屏，避免用空指针算几何。
    connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *removed) {
        if (!m_screen || m_screen == removed) {
            // screenRemoved can be emitted before Qt has moved the window off
            // the removed screen. Select only from the remaining outputs.
            const auto screens = qApp->screens();
            QScreen *fallback = nullptr;
            const auto primaryScreen = qApp->primaryScreen();
            for (auto *screen : screens) {
                if (screen == removed)
                    continue;
                if (!fallback || screen == primaryScreen)
                    fallback = screen;
                if (screen == primaryScreen)
                    break;
            }
            m_screen = fallback;
            onPositionChanged();
        }
    });

    connect(qApp, &QGuiApplication::screenRemoved, &m_exclusionZoneChangedTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    onScopeChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::scopeChanged, this, &LayerShellEmulation::onScopeChanged);

    onInputRegionChanged();
    connect(m_dlayerShellWindow, &DLayerShellWindow::inputRegionChanged, this, &LayerShellEmulation::onInputRegionChanged);

    // connect(m_dlayerShellWindow, &DS_NAMESPACE::DLayerShellWindow::keyboardInteractivityChanged, this, &LayerShellEmulation::onKeyboardInteractivityChanged);
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#STACKINGORDER
  * the following layered stacking order is recommended, from the bottom
  * _NET_WM_TYPE_DESKTOP
  * _NET_WM_STATE_BELOW
  * windows not belonging in any other layer
  * _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and windows having state _NET_WM_STATE_ABOVE
  * focused windows having state _NET_WM_STATE_FULLSCREEN
  */
void LayerShellEmulation::onLayerChanged()
{
    auto xcbWindow = dynamic_cast<QNativeInterface::Private::QXcbWindow*>(m_window->handle());
    switch (m_dlayerShellWindow->layer()) {
        case DLayerShellWindow::LayerBackground: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Desktop);
            break;
        }
        case DLayerShellWindow::LayerButtom: {
            //use Normal type will influenced by exclusionZone
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Normal);
            m_window->setFlags(Qt::WindowStaysOnBottomHint);
            break;
        }
        case DLayerShellWindow::LayerTop: {
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Dock);
            break;
        }
        case DLayerShellWindow::LayerOverlay: {
            // on deepin Notification will be influenced by exclusionZone,
            // while plasma works all right, maybe deepin kwin bug?
            // FIXME: fix above
            m_window->setFlags(m_window->flags() & ~Qt::WindowStaysOnBottomHint);
            xcbWindow->setWindowType(QNativeInterface::Private::QXcbWindow::Notification);
            break;
        }
    }
}

QScreen *LayerShellEmulation::targetScreen() const
{
    if (m_screen && qApp->screens().contains(m_screen.data())) {
        return m_screen.data();
    }
    // 尚未锁存或锁存的 output 已消失：仅使用仍然有效的窗口屏幕。
    auto *windowScreen = m_window ? m_window->screen() : nullptr;
    return windowScreen && qApp->screens().contains(windowScreen) ? windowScreen : nullptr;
}

void LayerShellEmulation::onPositionChanged()
{
    auto anchors = m_dlayerShellWindow->anchors();
    // 注意：这里必须用锁存的 output，不能用 m_window->screen()。
    // QWindow::screen() 是 Qt 依据窗口几何推导出来的，排列变化后窗口一旦
    // 落进相邻屏就会被判成"属于那块屏"，再把几何铺满那块屏，形成
    // "错位 → 判成邻屏 → 更错位" 的正反馈，副屏桌面会永久跑到主屏上。
    auto screen = targetScreen();
    if (!screen) {
        return;
    }

    int targetWidth = m_window->width();
    int targetHeight = m_window->height();
    if (m_dlayerShellWindow->preferredWidth() > 0) {
        targetWidth = m_dlayerShellWindow->preferredWidth();
    }
    if (m_dlayerShellWindow->preferredHeight() > 0) {
        targetHeight = m_dlayerShellWindow->preferredHeight();
    }

    auto screenRect = screen->geometry();
    auto x = screenRect.left() + (screenRect.width() - targetWidth) / 2;
    auto y = screenRect.top() + (screenRect.height() - targetHeight) / 2;
    if (anchors & DLayerShellWindow::AnchorRight) {
        // https://doc.qt.io/qt-6/qrect.html#right
        x = (screen->geometry().right() + 1 - targetWidth - m_dlayerShellWindow->rightMargin());
    }

    if (anchors & DLayerShellWindow::AnchorBottom) {
        // https://doc.qt.io/qt-6/qrect.html#bottom
        y = (screen->geometry().bottom() + 1 - targetHeight - m_dlayerShellWindow->bottomMargin());
    }
    if (anchors & DLayerShellWindow::AnchorLeft) {
        x = (screen->geometry().left() + m_dlayerShellWindow->leftMargin());
    }
    if (anchors & DLayerShellWindow::AnchorTop) {
        y = (screen->geometry().top() + m_dlayerShellWindow->topMargin());
    }

    QRect rect(x, y, targetWidth, targetHeight);

    const bool horizontallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorLeft, DLayerShellWindow::AnchorRight});
    const bool verticallyConstrained = anchors.testFlags({DLayerShellWindow::AnchorTop, DLayerShellWindow::AnchorBottom});

    if (horizontallyConstrained) {
        rect.setX(screen->geometry().left() + m_dlayerShellWindow->leftMargin());
        rect.setWidth(screen->geometry().width() - m_dlayerShellWindow->leftMargin() - m_dlayerShellWindow->rightMargin());
    }
    if (verticallyConstrained) {
        rect.setY(screen->geometry().top() + m_dlayerShellWindow->topMargin());
        rect.setHeight(screen->geometry().height() - m_dlayerShellWindow->topMargin() - m_dlayerShellWindow->bottomMargin());
    }

    if (m_window->geometry() != rect) {
        m_window->setGeometry(rect);
    }
}

/**
  * https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html#idm45649101327728
  */
void LayerShellEmulation::onExclusionZoneChanged()
{
    // dde-shell issues:379
    if (m_dlayerShellWindow->exclusionZone() < 0)
        return;
    auto scaleFactor = qGuiApp->devicePixelRatio();
    auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    xcb_ewmh_connection_t ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(x11Application->connection(), &ewmh_connection);
    xcb_ewmh_init_atoms_replies(&ewmh_connection, cookie, NULL);
    xcb_ewmh_wm_strut_partial_t strut_partial;
    memset(&strut_partial, 0, sizeof(xcb_ewmh_wm_strut_partial_t));
    auto anchors = m_dlayerShellWindow->anchors();
    QScreen *currentScreen = targetScreen();
    if (!currentScreen) {
        return;
    }
    if ((anchors == DLayerShellWindow::AnchorLeft) || (anchors ^ DLayerShellWindow::AnchorLeft) == (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom)) {        
        // 计算独占区域：屏幕X坐标 + 任务栏物理宽度
        // 注意：QScreen::geometry().x() 已经是设备无关像素，不需要缩放
        // 只有exclusionZone需要转换为物理像素
        strut_partial.left = static_cast<uint32_t>(currentScreen->geometry().x() + m_dlayerShellWindow->exclusionZone() * scaleFactor);
        strut_partial.left_start_y = static_cast<uint32_t>(m_window->geometry().y());
        strut_partial.left_end_y = static_cast<uint32_t>(m_window->geometry().y() + m_window->geometry().height() * scaleFactor);

        qCDebug(layershell) << "AnchorLeft: screen.x=" << currentScreen->geometry().x() << "exclusionZone=" << m_dlayerShellWindow->exclusionZone()
                            << "result=" << strut_partial.left;
    } else if ((anchors == DLayerShellWindow::AnchorRight) || (anchors ^ DLayerShellWindow::AnchorRight) == (DLayerShellWindow::AnchorTop | DLayerShellWindow::AnchorBottom)) {
        // 找到桌面的最右边界（物理像素）
        // 注意：QScreen::geometry()返回混合坐标系统 - X/Y是物理像素，宽度/高度是逻辑像素
        int desktopRightBoundaryPhysical = 0;
        for (auto screen : qApp->screens()) {
            // 屏幕物理右边界 = X坐标(物理) + 宽度(逻辑) * 缩放系数
            int screenRightPhysical = screen->geometry().x() + static_cast<int>(screen->geometry().width() * scaleFactor);
            if (desktopRightBoundaryPhysical < screenRightPhysical)
                desktopRightBoundaryPhysical = screenRightPhysical;
        }

        // 计算当前屏幕物理右边界
        int currentScreenRightPhysical = currentScreen->geometry().x() + static_cast<int>(currentScreen->geometry().width() * scaleFactor);

        // 计算到桌面右边界的物理距离
        int distanceToDesktopRightPhysical = desktopRightBoundaryPhysical - currentScreenRightPhysical;

        // 独占区域 = 到桌面右边界的物理距离 + 任务栏物理宽度
        strut_partial.right = static_cast<uint32_t>(distanceToDesktopRightPhysical + m_dlayerShellWindow->exclusionZone() * scaleFactor);

        qCDebug(layershell) << "AnchorRight: desktopRightBoundary=" << desktopRightBoundaryPhysical << "currentScreenRight=" << currentScreenRightPhysical
                            << "distance=" << distanceToDesktopRightPhysical << "result=" << strut_partial.right;

        strut_partial.right_start_y = static_cast<uint32_t>(m_window->geometry().y());
        strut_partial.right_end_y = static_cast<uint32_t>(m_window->geometry().y() + m_window->geometry().height() * scaleFactor);
    } else if ((anchors == DLayerShellWindow::AnchorTop) || (anchors ^ DLayerShellWindow::AnchorTop) == (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight)) {        
        // 计算独占区域：屏幕Y坐标 + 任务栏物理高度
        // 注意：QScreen::geometry().y() 已经是设备无关像素，不需要缩放
        // 只有exclusionZone需要转换为物理像素
        strut_partial.top = static_cast<uint32_t>(currentScreen->geometry().y() + m_dlayerShellWindow->exclusionZone() * scaleFactor);
        strut_partial.top_start_x = static_cast<uint32_t>(m_window->geometry().x());
        strut_partial.top_end_x = static_cast<uint32_t>(m_window->geometry().x() + m_window->geometry().width() * scaleFactor);

        qCDebug(layershell) << "AnchorTop: screen.y=" << currentScreen->geometry().y() << "exclusionZone=" << m_dlayerShellWindow->exclusionZone()
                            << "result=" << strut_partial.top;
    } else if ((anchors == DLayerShellWindow::AnchorBottom) || (anchors ^ DLayerShellWindow::AnchorBottom) == (DLayerShellWindow::AnchorLeft | DLayerShellWindow::AnchorRight)) {
        // 计算当前屏幕的物理底边界（修正混合坐标系统）
        int currentScreenBottomPhysical = currentScreen->geometry().y() + static_cast<int>(currentScreen->geometry().height() * scaleFactor);

        // 查找紧邻下方的屏幕，支持垂直布局
        // 算法：找到在当前屏幕下方且与当前屏幕水平重叠的屏幕
        int belowScreensHeight = 0;
        QRect currentRect = currentScreen->geometry();
        for (auto screen : qApp->screens()) {
            if (screen == currentScreen)
                continue;
            QRect screenRect = screen->geometry();

            // 检查屏幕是否在当前屏幕下方（使用物理坐标）
            if (screenRect.y() >= currentScreenBottomPhysical) {
                // 检查是否有水平重叠（支持垂直布局）
                // 使用物理坐标计算重叠
                int screenLeftPhysical = screenRect.x();
                int screenRightPhysical = screenRect.x() + static_cast<int>(screenRect.width() * scaleFactor);
                int currentLeftPhysical = currentRect.x();
                int currentRightPhysical = currentRect.x() + static_cast<int>(currentRect.width() * scaleFactor);
                bool hasHorizontalOverlap = (screenLeftPhysical < currentRightPhysical && screenRightPhysical > currentLeftPhysical);
                if (hasHorizontalOverlap) {
                    // 累加下方屏幕的高度（逻辑像素）
                    belowScreensHeight += screenRect.height();
                    qCDebug(layershell) << "Found below screen:" << screenRect.height() << "px";
                }
            }
        }

        // 如果没有找到下方屏幕，使用修正的回退算法（支持水平布局）
        if (belowScreensHeight == 0) {
            // 找到桌面最底边的物理边界
            int desktopBottomBoundaryPhysical = 0;
            for (auto screen : qApp->screens()) {
                int screenBottomPhysical = screen->geometry().y() + static_cast<int>(screen->geometry().height() * scaleFactor);
                if (desktopBottomBoundaryPhysical < screenBottomPhysical)
                    desktopBottomBoundaryPhysical = screenBottomPhysical;
            }

            // 计算物理距离，然后转换为逻辑像素用于后续计算
            int distancePhysical = desktopBottomBoundaryPhysical - currentScreenBottomPhysical;
            belowScreensHeight = static_cast<int>(distancePhysical / scaleFactor);
        }

        // 独占区域 = 下方区域物理高度 + 任务栏物理高度
        strut_partial.bottom = static_cast<uint32_t>(belowScreensHeight * scaleFactor + m_dlayerShellWindow->exclusionZone() * scaleFactor);

        qCDebug(layershell) << "AnchorBottom: belowScreensHeight=" << belowScreensHeight << "exclusionZone=" << m_dlayerShellWindow->exclusionZone()
                            << "result=" << strut_partial.bottom;

        strut_partial.bottom_start_x = static_cast<uint32_t>(m_window->geometry().x());
        strut_partial.bottom_end_x = static_cast<uint32_t>(m_window->geometry().x() + m_window->geometry().width() * scaleFactor);
    }

    qCDebug(layershell) << "update exclusion zone, winId:" << m_window->winId()
                        << ", (left, right, top, bottom)"
                        << strut_partial.left << strut_partial.right << strut_partial.top << strut_partial.bottom;
    xcb_ewmh_set_wm_strut_partial(&ewmh_connection, m_window->winId(), strut_partial);
}

void LayerShellEmulation::onScopeChanged()
{
    auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11Application || !m_window->winId()) {
        return;
    }

    QString instanceName = m_dlayerShellWindow->scope();
    if (instanceName.isEmpty()) {
        // Don't set WM_CLASS for default scope, let X11 use its default behavior
        return;
    }

    // Set WM_CLASS property
    // Format: instance_name\0class_name\0
    // - instance_name: scope value (e.g., "dock", "notification")
    // - class_name: application binary name (e.g., "dde-shell")
    QString className = QCoreApplication::applicationName();
    QByteArray wmClassData;
    wmClassData.append(instanceName.toUtf8());
    wmClassData.append('\0');
    wmClassData.append(className.toUtf8());
    wmClassData.append('\0');

    xcb_icccm_set_wm_class(x11Application->connection(), m_window->winId(), wmClassData.length(), wmClassData.constData());

    qCDebug(layershell) << "Set WM_CLASS for window" << m_window->winId() << " wm_class:" << wmClassData;
}

void LayerShellEmulation::onInputRegionChanged()
{
    auto *x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11Application || !m_window->winId() || !m_dlayerShellWindow) {
        return;
    }

    if (m_dlayerShellWindow->inputRegion().isNull()) {
        // Reset input region (no shape constraint)
        xcb_shape_mask(x11Application->connection(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, m_window->winId(), 0, 0, XCB_NONE);
        xcb_flush(x11Application->connection());
        return;
    }

    QRegion region = m_dlayerShellWindow->inputRegion();
    qreal scaleFactor = qGuiApp->devicePixelRatio();

    QVector<xcb_rectangle_t> rects;
    for (const QRect &r : region) {
        xcb_rectangle_t rect;
        rect.x = r.x() * scaleFactor;
        rect.y = r.y() * scaleFactor;
        rect.width = r.width() * scaleFactor;
        rect.height = r.height() * scaleFactor;
        rects.append(rect);
    }

    // Set the input shape via XCB
    // If rects vector is empty, the window will become completely transparent to input clicks (unclickable)
    xcb_shape_rectangles(x11Application->connection(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT,
                         XCB_CLIP_ORDERING_UNSORTED, m_window->winId(), 0, 0,
                         rects.size(), rects.data());
    xcb_flush(x11Application->connection());
}

// void X11Emulation::onKeyboardInteractivityChanged()
// {
//     // kwin no implentation on wayland
// }
DS_END_NAMESPACE
