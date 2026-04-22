// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockpanel.h"
#include "constants.h"
#include "dockadaptor.h"
#include "docksettings.h"
#include "panel.h"
#include "pluginfactory.h"
#include "waylanddockhelper.h"
#include "x11dockhelper.h"

// for old api compatible
#include "dockdbusproxy.h"
#include "dockfrontadaptor.h"
#include "dockdaemonadaptor.h"
#include "loadtrayplugins.h"
#include <DDBusSender>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QGuiApplication>
#include <QQuickItem>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QIcon>
#include <DGuiApplicationHelper>
#include <DPlatformTheme>

#define SETTINGS DockSettings::instance()

Q_LOGGING_CATEGORY(dockLog, "org.deepin.dde.shell.dock")

namespace dock {

namespace {
constexpr auto kAppearanceService = "org.deepin.dde.Appearance1";
constexpr auto kAppearancePath = "/org/deepin/dde/Appearance1";
constexpr auto kAppearanceInterface = "org.deepin.dde.Appearance1";
constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties";

Dtk::Gui::DGuiApplicationHelper::ColorType explicitThemeTypeFromName(const QString &themeName)
{
    const QString normalizedThemeName = themeName.trimmed().toLower();
    if (normalizedThemeName.isEmpty())
        return Dtk::Gui::DGuiApplicationHelper::UnknownType;

    if (normalizedThemeName == QStringLiteral("dark")
        || normalizedThemeName.endsWith(QStringLiteral(".dark"))
        || normalizedThemeName.endsWith(QStringLiteral("-dark"))
        || normalizedThemeName.endsWith(QStringLiteral("_dark"))) {
        return Dtk::Gui::DGuiApplicationHelper::DarkType;
    }

    if (normalizedThemeName == QStringLiteral("light")
        || normalizedThemeName.endsWith(QStringLiteral(".light"))
        || normalizedThemeName.endsWith(QStringLiteral("-light"))
        || normalizedThemeName.endsWith(QStringLiteral("_light"))) {
        return Dtk::Gui::DGuiApplicationHelper::LightType;
    }

    return Dtk::Gui::DGuiApplicationHelper::UnknownType;
}

Dtk::Gui::DGuiApplicationHelper::ColorType currentEffectiveThemeType(Dtk::Gui::DGuiApplicationHelper *helper)
{
    if (!helper)
        return Dtk::Gui::DGuiApplicationHelper::LightType;

    const auto currentThemeType = helper->themeType();
    if (currentThemeType != Dtk::Gui::DGuiApplicationHelper::UnknownType)
        return currentThemeType;

    const auto currentPaletteType = helper->paletteType();
    if (currentPaletteType != Dtk::Gui::DGuiApplicationHelper::UnknownType)
        return currentPaletteType;

    return Dtk::Gui::DGuiApplicationHelper::LightType;
}

Dtk::Gui::DGuiApplicationHelper::ColorType effectiveThemeTypeFromAppearance(
    const QString &globalThemeName,
    const QString &gtkThemeName,
    const QString &iconThemeName,
    Dtk::Gui::DGuiApplicationHelper *helper)
{
    const auto globalThemeType = explicitThemeTypeFromName(globalThemeName);
    if (globalThemeType != Dtk::Gui::DGuiApplicationHelper::UnknownType)
        return globalThemeType;

    const auto gtkThemeType = explicitThemeTypeFromName(gtkThemeName);
    if (gtkThemeType != Dtk::Gui::DGuiApplicationHelper::UnknownType)
        return gtkThemeType;

    const auto iconThemeType = explicitThemeTypeFromName(iconThemeName);
    if (iconThemeType != Dtk::Gui::DGuiApplicationHelper::UnknownType)
        return iconThemeType;

    return currentEffectiveThemeType(helper);
}

ColorTheme colorThemeFromHelperThemeType(Dtk::Gui::DGuiApplicationHelper::ColorType themeType)
{
    return themeType == Dtk::Gui::DGuiApplicationHelper::DarkType ? Dark : Light;
}

QString readAppearanceStringProperty(const QString &propertyName)
{
    QDBusInterface appearanceProperties(kAppearanceService,
                                        kAppearancePath,
                                        kPropertiesInterface,
                                        QDBusConnection::sessionBus());
    if (!appearanceProperties.isValid())
        return QString();

    const QDBusReply<QDBusVariant> reply = appearanceProperties.call(QStringLiteral("Get"),
                                                                     QString::fromLatin1(kAppearanceInterface),
                                                                     propertyName);
    return reply.isValid() ? reply.value().variant().toString() : QString();
}

void syncApplicationTheme(Dtk::Gui::DGuiApplicationHelper *helper,
                          const QString &globalThemeName,
                          const QString &gtkThemeName,
                          const QString &iconThemeName)
{
    if (!helper)
        return;

    const auto explicitThemeType = explicitThemeTypeFromName(globalThemeName);
    const bool followSystemTheme = explicitThemeType == Dtk::Gui::DGuiApplicationHelper::UnknownType;
    const auto targetPaletteType = followSystemTheme ? Dtk::Gui::DGuiApplicationHelper::UnknownType : explicitThemeType;

    if (auto *applicationTheme = helper->applicationTheme()) {
        const QByteArray gtkThemeNameBytes = gtkThemeName.toUtf8();
        if (!gtkThemeNameBytes.isEmpty() && applicationTheme->themeName() != gtkThemeNameBytes) {
            applicationTheme->setThemeName(gtkThemeNameBytes);
        }

        const QByteArray iconThemeNameBytes = iconThemeName.toUtf8();
        if (!iconThemeNameBytes.isEmpty() && applicationTheme->iconThemeName() != iconThemeNameBytes) {
            applicationTheme->setIconThemeName(iconThemeNameBytes);
        }
    }

    if (helper->paletteType() != targetPaletteType) {
        helper->setPaletteType(targetPaletteType);
    }

    const auto targetThemeType = effectiveThemeTypeFromAppearance(globalThemeName, gtkThemeName, iconThemeName, helper);
    const QPalette targetPalette = helper->applicationPalette(targetThemeType);
    if (helper->applicationPalette() != targetPalette) {
        helper->setApplicationPalette(targetPalette);
    }

    if (!iconThemeName.isEmpty() && QIcon::themeName() != iconThemeName) {
        QIcon::setThemeName(iconThemeName);
    }
}
}

DockPanel::DockPanel(QObject *parent)
    : DPanel(parent)
    , m_theme(ColorTheme::Dark)
    , m_hideState(Show)
    , m_dockScreen(nullptr)
    , m_loadTrayPlugins(new LoadTrayPlugins(this))
    , m_compositorReady(false)
    , m_launcherShown(false)
    , m_themeSyncTimer(new QTimer(this))
    , m_contextDragging(false)
    , m_containsMouse(false)
    , m_reportedContainsMouse(false)
    , m_isResizing(false)
    , m_cursorPosition(0, 0)
    , m_reportedCursorPosition(0, 0)
{
    m_themeSyncTimer->setSingleShot(true);
    m_themeSyncTimer->setInterval(80);
    connect(m_themeSyncTimer, &QTimer::timeout, this, &DockPanel::syncColorThemeWithSystem);

    connect(this, &DockPanel::compositorReadyChanged, this, [this] {
        if (!m_compositorReady) return;
        m_loadTrayPlugins->loadDockPlugins();
    });
}

bool DockPanel::load()
{
    DPanel::load();
    return true;
}

bool DockPanel::init()
{
    DockAdaptor* adaptor = new DockAdaptor(this);
    Q_UNUSED(adaptor)
    QDBusConnection::sessionBus().registerService("org.deepin.ds.Dock");
    QDBusConnection::sessionBus().registerObject("/org/deepin/ds/Dock", "org.deepin.ds.Dock", this);

    // for old api compatible
    DockDBusProxy* proxy = new DockDBusProxy(this);
    DockFrontAdaptor* dockFrontAdaptor = new DockFrontAdaptor(proxy);
    Q_UNUSED(dockFrontAdaptor)
    QDBusConnection::sessionBus().registerService("org.deepin.dde.Dock1");
    QDBusConnection::sessionBus().registerObject("/org/deepin/dde/Dock1", "org.deepin.dde.Dock1", proxy);

    DockDaemonAdaptor* dockDaemonAdaptor = new DockDaemonAdaptor(proxy);
    QDBusConnection::sessionBus().registerService("org.deepin.dde.daemon.Dock1");
    QDBusConnection::sessionBus().registerObject("/org/deepin/dde/daemon/Dock1", "org.deepin.dde.daemon.Dock1", proxy);
    connect(this, &DockPanel::rootObjectChanged, this, [this](){
        connect(window(), &QWindow::screenChanged, this, [ = ] {
            // FIXME: find why window screen changed and fix it
            if (m_dockScreen) {
                if (m_dockScreen != window()->screen() && qApp->screens().contains( m_dockScreen)) {
                    qWarning() << "m_dockScreen" << m_dockScreen << m_dockScreen->name() << "window()->screen()" << window()->screen() << window()->screen()->name();
                    QTimer::singleShot(10, this, [this](){
                        window()->setScreen(m_dockScreen);
                        onWindowGeometryChanged();
                    });
                } else {
                    onWindowGeometryChanged();
                }
            }else {
                m_dockScreen = window()->screen();
            }
        });
    });
    connect(this, &DockPanel::hideModeChanged, this, [this](){
        if (hideMode() == KeepShowing)
            setHideState(Show);
    });
    connect(SETTINGS, &DockSettings::positionChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT positionChanged(position());
        Q_EMIT dockDaemonAdaptor->PositionChanged(position());
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());

        QMetaObject::invokeMethod(this,[this](){
            Q_EMIT onWindowGeometryChanged();
        });
    });
    connect(SETTINGS, &DockSettings::showInPrimaryChanged, this, [this, dockDaemonAdaptor](){
        updateDockScreen();
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());
        Q_EMIT showInPrimaryChanged(showInPrimary());
    });

    connect(this, &DockPanel::frontendWindowRectChanged, dockDaemonAdaptor, &DockDaemonAdaptor::FrontendWindowRectChanged);
    connect(SETTINGS, &DockSettings::dockSizeChanged, this, &DockPanel::dockSizeChanged);
    connect(SETTINGS, &DockSettings::hideModeChanged, this, &DockPanel::hideModeChanged);
    connect(SETTINGS, &DockSettings::viewModeChanged, this, &DockPanel::viewModeChanged);
    connect(SETTINGS, &DockSettings::itemAlignmentChanged, this, &DockPanel::itemAlignmentChanged);
    connect(SETTINGS, &DockSettings::indicatorStyleChanged, this, &DockPanel::indicatorStyleChanged);
    connect(SETTINGS, &DockSettings::lockedChanged, this, &DockPanel::lockedChanged);

    connect(SETTINGS, &DockSettings::dockSizeChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT dockDaemonAdaptor->WindowSizeEfficientChanged(dockSize());
        Q_EMIT dockDaemonAdaptor->WindowSizeFashionChanged(dockSize());
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());
    });
    connect(SETTINGS, &DockSettings::hideModeChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT dockDaemonAdaptor->HideModeChanged(hideMode());
    });
    connect(SETTINGS, &DockSettings::itemAlignmentChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT dockDaemonAdaptor->DisplayModeChanged(itemAlignment());
    });
    connect(SETTINGS, &DockSettings::lockedChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT dockDaemonAdaptor->LockedChanged(locked());
    });

    DPanel::init();

    QObject::connect(this, &DApplet::rootObjectChanged, this, [this]() {
        if (rootObject()) {
            // those connections need connect after DPanel::init() which create QQuickWindow
            if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
                connect(window(), &QQuickWindow::xChanged, this, &DockPanel::onWindowGeometryChanged);
                connect(window(), &QQuickWindow::yChanged, this, &DockPanel::onWindowGeometryChanged);
            }
            connect(window(), &QQuickWindow::widthChanged, this, &DockPanel::onWindowGeometryChanged);
            connect(window(), &QQuickWindow::heightChanged, this, &DockPanel::onWindowGeometryChanged);
            QMetaObject::invokeMethod(this, &DockPanel::onWindowGeometryChanged);
            if (showInPrimary())
                updateDockScreen();
            else {
                m_dockScreen = window()->screen();
            }
            rootObject()->installEventFilter(this);
            Q_EMIT devicePixelRatioChanged(window()->devicePixelRatio());
        }
    });

    auto *guiHelper = Dtk::Gui::DGuiApplicationHelper::instance();
    QObject::connect(guiHelper, &Dtk::Gui::DGuiApplicationHelper::themeTypeChanged,
                     this, &DockPanel::syncColorThemeWithSystem);
    if (auto *platformTheme = guiHelper->applicationTheme()) {
        QObject::connect(platformTheme, &Dtk::Gui::DPlatformTheme::themeNameChanged,
                         this, &DockPanel::syncColorThemeWithSystem);
    }
    QDBusConnection::sessionBus().connect(kAppearanceService, kAppearancePath, kAppearanceInterface,
                                          "Changed", this, SLOT(onAppearanceChanged(QString,QString)));
    QDBusConnection::sessionBus().connect(kAppearanceService, kAppearancePath, kAppearanceInterface,
                                          "Refreshed", this, SLOT(onAppearanceRefreshed(QString)));
    syncColorThemeWithSystem();

    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        m_helper = new WaylandDockHelper(this);
    } else if (QStringLiteral("xcb") == platformName) {
        m_helper = new X11DockHelper(this);
    }

    QMetaObject::invokeMethod(this, [this, dockDaemonAdaptor]() {
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());
    });

    // TODO: get launchpad status from applet not dbus
    QDBusConnection::sessionBus().connect("org.deepin.dde.Launcher1", "/org/deepin/dde/Launcher1", "org.deepin.dde.Launcher1", "VisibleChanged", this, SLOT(launcherVisibleChanged(bool)));
    if (showInPrimary())
        connect(qApp, &QGuiApplication::primaryScreenChanged, this, &DockPanel::updateDockScreen, Qt::UniqueConnection);

    return true;
}

QRect DockPanel::geometry()
{
    Q_ASSERT(window());
    return window()->geometry();
}

QRect DockPanel::frontendWindowRect()
{
    return m_frontendWindowRect;
}

void DockPanel::setFrontendWindowRect(int transformOffsetX, int transformOffsetY)
{
    if(!window()) return;

    auto ratio = window()->devicePixelRatio();
    auto screenGeometry = window()->screen()->geometry();
    auto geometry = window()->geometry();
    const int xOffset = geometry.x() - screenGeometry.x() + transformOffsetX;
    const int yOffset = geometry.y() - screenGeometry.y() + transformOffsetY;

    m_frontendWindowRect = QRect(screenGeometry.x() + xOffset * ratio,
                                 screenGeometry.y() + yOffset * ratio,
                                 geometry.width() * ratio,
                                 geometry.height() * ratio);
}

ColorTheme DockPanel::colorTheme()
{
    return m_theme;
}

void DockPanel::setColorTheme(const ColorTheme& theme)
{
    if (theme == m_theme)
        return;
    m_theme = theme;
    Q_EMIT this->colorThemeChanged(theme);
}

void DockPanel::onAppearanceChanged(const QString &type, const QString &value)
{
    Q_UNUSED(value)
    static const QStringList kTrackedAppearanceKeys{
        QStringLiteral("GlobalTheme"),
        QStringLiteral("GtkTheme"),
        QStringLiteral("IconTheme"),
        QStringLiteral("QtActiveColor")
    };

    if (!kTrackedAppearanceKeys.contains(type, Qt::CaseInsensitive))
        return;

    m_themeSyncTimer->start();
}

void DockPanel::onAppearanceRefreshed(const QString &type)
{
    static const QStringList kTrackedAppearanceKeys{
        QStringLiteral("GlobalTheme"),
        QStringLiteral("GtkTheme"),
        QStringLiteral("IconTheme"),
        QStringLiteral("QtActiveColor")
    };

    if (!type.isEmpty() && !kTrackedAppearanceKeys.contains(type, Qt::CaseInsensitive))
        return;

    m_themeSyncTimer->start();
}

void DockPanel::syncColorThemeWithSystem()
{
    auto *guiHelper = Dtk::Gui::DGuiApplicationHelper::instance();
    QDBusInterface appearanceProperties(kAppearanceService,
                                        kAppearancePath,
                                        kPropertiesInterface,
                                        QDBusConnection::sessionBus());
    if (appearanceProperties.isValid()) {
        const QDBusReply<QDBusVariant> reply = appearanceProperties.call(QStringLiteral("Get"),
                                                                         QString::fromLatin1(kAppearanceInterface),
                                                                         QStringLiteral("GlobalTheme"));
        if (reply.isValid()) {
            const QString globalThemeName = reply.value().variant().toString();
            const QString gtkThemeName = readAppearanceStringProperty(QStringLiteral("GtkTheme"));
            const QString iconThemeName = readAppearanceStringProperty(QStringLiteral("IconTheme"));
            syncApplicationTheme(guiHelper, globalThemeName, gtkThemeName, iconThemeName);
            setColorTheme(colorThemeFromHelperThemeType(
                effectiveThemeTypeFromAppearance(globalThemeName, gtkThemeName, iconThemeName, guiHelper)));
            return;
        }
    }

    syncApplicationTheme(guiHelper, QString(), QString(), QString());
    setColorTheme(colorThemeFromHelperThemeType(currentEffectiveThemeType(guiHelper)));
}

uint DockPanel::dockSize()
{
    return SETTINGS->dockSize();
}

void DockPanel::setDockSize(const uint& size)
{
    if (size > MAX_DOCK_SIZE || size < MIN_DOCK_SIZE) {
        return;
    }

    SETTINGS->setDockSize(size);
}

HideMode DockPanel::hideMode()
{
    return SETTINGS->hideMode();
}

void DockPanel::setHideMode(const HideMode& mode)
{
    SETTINGS->setHideMode(mode);
    Q_EMIT hideStateChanged(hideState());
}

Position DockPanel::position()
{
    return SETTINGS->position();
}

ViewMode DockPanel::viewMode()
{
    return SETTINGS->viewMode();
}

void DockPanel::setPosition(const Position& position)
{
    if (position == SETTINGS->position()) return;

    // Emit signal with old position before updating
    Q_EMIT beforePositionChanged(SETTINGS->position());

    // Directly commit the position change
    SETTINGS->setPosition(position);
}

void DockPanel::setViewMode(const ViewMode &mode)
{
    if (SETTINGS->viewMode() == mode)
        return;

    SETTINGS->setViewMode(mode);

    switch (mode) {
    case ViewMode::CenteredMode:
        SETTINGS->setItemAlignment(ItemAlignment::CenterAlignment);
        break;
    case ViewMode::LeftAlignedMode:
        SETTINGS->setItemAlignment(ItemAlignment::LeftAlignment);
        break;
    case ViewMode::FashionMode:
        SETTINGS->setItemAlignment(ItemAlignment::LeftAlignment);
        SETTINGS->setIndicatorStyle(IndicatorStyle::Fashion);
        break;
    }
}

ItemAlignment DockPanel::itemAlignment()
{
    return SETTINGS->itemAlignment();
}

void DockPanel::setItemAlignment(const ItemAlignment& alignment)
{
    if (SETTINGS->viewMode() != ViewMode::FashionMode) {
        SETTINGS->setViewMode(alignment == ItemAlignment::CenterAlignment ? ViewMode::CenteredMode : ViewMode::LeftAlignedMode);
    }
    SETTINGS->setItemAlignment(alignment);
}

IndicatorStyle DockPanel::indicatorStyle()
{
    return SETTINGS->indicatorStyle();
}

void DockPanel::setIndicatorStyle(const IndicatorStyle& style)
{
    SETTINGS->setIndicatorStyle(style);
}

void DockPanel::onWindowGeometryChanged()
{
    setFrontendWindowRect(0, 0);
    Q_EMIT frontendWindowRectChanged(m_frontendWindowRect);
    Q_EMIT geometryChanged(geometry());
}

bool DockPanel::compositorReady()
{
    return m_compositorReady;
}

void DockPanel::setCompositorReady(bool ready)
{
    if (ready == m_compositorReady) {
        return;
    }

    m_compositorReady = ready;
    Q_EMIT compositorReadyChanged();
}

HideState DockPanel::hideState()
{
    if (m_launcherShown) {
        return Show;
    }

    return m_hideState;
}

void DockPanel::ReloadPlugins()
{
// TODO: implement this function
}

void DockPanel::callShow()
{
// TODO: implement this function
}

bool DockPanel::debugMode() const
{
#ifndef QT_DEBUG
    return false;
#else
    return true;
#endif
}

void DockPanel::openDockSettings() const
{
    DDBusSender()
        .service(QStringLiteral("org.deepin.dde.ControlCenter1"))
        .path(QStringLiteral("/org/deepin/dde/ControlCenter1"))
        .interface(QStringLiteral("org.deepin.dde.ControlCenter1"))
        .method(QStringLiteral("ShowPage"))
        .arg(QStringLiteral("personalization/dock"))
        .call();
}

void DockPanel::notifyDockPositionChanged(int offsetX, int offsetY)
{
    setFrontendWindowRect(offsetX, offsetY);
    Q_EMIT frontendWindowRectChanged(m_frontendWindowRect);
}

void DockPanel::reportMousePresence(bool containsMouse, const QPointF &cursorPosition)
{
    const bool previousContainsMouse = this->containsMouse();
    const QPointF previousCursorPosition = this->cursorPosition();

    const auto isSameReportedPosition = [](const QPointF &lhs, const QPointF &rhs) {
        constexpr qreal epsilon = 0.5;
        return qAbs(lhs.x() - rhs.x()) <= epsilon && qAbs(lhs.y() - rhs.y()) <= epsilon;
    };

    // Ignore delayed clear requests from a previously hovered delegate once a new
    // delegate has already taken over spotlight reporting.
    if (!containsMouse && m_reportedContainsMouse && cursorPosition != QPointF() && !isSameReportedPosition(m_reportedCursorPosition, cursorPosition)) {
        return;
    }

    m_reportedContainsMouse = containsMouse;
    if (containsMouse) {
        m_reportedCursorPosition = cursorPosition;
    }

    if (previousContainsMouse != this->containsMouse()) {
        emit containsMouseChanged(this->containsMouse());
    }
    if (previousCursorPosition != this->cursorPosition()) {
        emit cursorPositionChanged(this->cursorPosition());
    }
}

void DockPanel::launcherVisibleChanged(bool visible)
{
    if (visible == m_launcherShown) return;

    const HideState oldHideState = hideState();
    m_launcherShown = visible;
    const HideState newHideState = hideState();

    if (newHideState != oldHideState) {
        Q_EMIT hideStateChanged(newHideState);
    }
}

void DockPanel::updateDockScreen()
{
    auto win = window();
    if (!win)
        return;
    setDockScreen(qApp->primaryScreen());
}

bool DockPanel::showInPrimary() const
{
    return SETTINGS->showInPrimary();
}

void DockPanel::setShowInPrimary(bool newShowInPrimary)
{
    if (SETTINGS->showInPrimary() == newShowInPrimary)
        return;
    SETTINGS->setShowInPrimary(newShowInPrimary);
    if (newShowInPrimary)
        connect(qApp, &QGuiApplication::primaryScreenChanged, this, &DockPanel::updateDockScreen, Qt::UniqueConnection);
    else
        disconnect(qApp, &QGuiApplication::primaryScreenChanged, this, &DockPanel::updateDockScreen);
}

bool DockPanel::locked() const
{
    return SETTINGS->locked();
}

void DockPanel::setLocked(bool newLocked)
{
    SETTINGS->setLocked(newLocked);
}

D_APPLET_CLASS(DockPanel)

void DockPanel::setHideState(HideState newHideState)
{
    if (m_hideState == newHideState)
        return;
    m_hideState = newHideState;
    Q_EMIT hideStateChanged(m_hideState);
}

QScreen * DockPanel::dockScreen()
{
    return m_dockScreen;
}

void DockPanel::setDockScreen(QScreen *screen)
{
    if (m_dockScreen == screen)
        return;
    m_dockScreen = screen;
    window()->setScreen(m_dockScreen);
    Q_EMIT dockScreenChanged(m_dockScreen);
    Q_EMIT screenNameChanged();
}

QString DockPanel::screenName() const
{
    if (!m_dockScreen)
        return {};
    return m_dockScreen->name();
}

qreal DockPanel::devicePixelRatio() const
{
    if (!window())
        return 1.0;
    return window()->devicePixelRatio();
}

bool DockPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == window() && event->type() == QEvent::DevicePixelRatioChange) {
        Q_EMIT devicePixelRatioChanged(window()->devicePixelRatio());
    }

    return false;
}

bool DockPanel::contextDragging() const
{
    return m_contextDragging;
}

void DockPanel::setContextDragging(bool newContextDragging)
{
    if (m_contextDragging == newContextDragging)
        return;
    m_contextDragging = newContextDragging;
    if (!m_contextDragging)
        m_helper->checkNeedHideOrNot();
    emit contextDraggingChanged();
}

bool DockPanel::containsMouse() const
{
    return m_containsMouse || m_reportedContainsMouse;
}

QPointF DockPanel::cursorPosition() const
{
    return m_reportedContainsMouse ? m_reportedCursorPosition : m_cursorPosition;
}

void DockPanel::setContainsMouse(bool containsMouse)
{
    const bool previousContainsMouse = this->containsMouse();
    const QPointF previousCursorPosition = this->cursorPosition();
    if (m_containsMouse == containsMouse)
        return;

    m_containsMouse = containsMouse;
    if (previousContainsMouse != this->containsMouse()) {
        emit containsMouseChanged(this->containsMouse());
    }
    if (previousCursorPosition != this->cursorPosition()) {
        emit cursorPositionChanged(this->cursorPosition());
    }
}

void DockPanel::setCursorPosition(const QPointF &cursorPosition)
{
    const QPointF previousCursorPosition = this->cursorPosition();
    if (m_cursorPosition == cursorPosition)
        return;

    m_cursorPosition = cursorPosition;
    if (previousCursorPosition != this->cursorPosition()) {
        emit cursorPositionChanged(this->cursorPosition());
    }
}

bool DockPanel::isResizing() const
{
    return m_isResizing;

}

void DockPanel::setIsResizing(bool resizing)
{
    if (m_isResizing == resizing)
        return;
    m_isResizing = resizing;
    emit isResizingChanged(m_isResizing);
}
}

#include "dockpanel.moc"
