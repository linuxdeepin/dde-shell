// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "constants.h"
#include "dockpanel.h"
#include "dockadaptor.h"
#include "environments.h"
#include "docksettings.h"
#include "pluginfactory.h"
#include "x11dockhelper.h"
#include "waylanddockhelper.h"

// for old api compatible
#include "dockdbusproxy.h"
#include "dockfrontadaptor.h"
#include "dockdaemonadaptor.h"

#include <DDBusSender>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QGuiApplication>
#include <QQuickItem>
#include <DGuiApplicationHelper>
#include <qt5/QtGui/qguiapplication.h>

#define SETTINGS DockSettings::instance()

Q_LOGGING_CATEGORY(dockLog, "dde.shell.dock")

namespace dock {

DockPanel::DockPanel(QObject * parent)
    : DPanel(parent)
    , m_theme(ColorTheme::Dark)
    , m_hideState(Hide)
    , m_compositorReady(false)
    , m_launcherShown(false)
{
    connect(this, &DockPanel::compositorReadyChanged, this, &DockPanel::loadDockPlugins);
}

bool DockPanel::load()
{
    DPanel::load();
    return true;
}

bool DockPanel::init()
{
    DockAdaptor* adaptor = new DockAdaptor(this);
    QDBusConnection::sessionBus().registerService("org.deepin.ds.Dock");
    QDBusConnection::sessionBus().registerObject("/org/deepin/ds/Dock", "org.deepin.ds.Dock", this);

    // for old api compatible
    DockDBusProxy* proxy = new DockDBusProxy(this);
    DockFrontAdaptor* dockFrontAdaptor = new DockFrontAdaptor(proxy);
    QDBusConnection::sessionBus().registerService("org.deepin.dde.Dock1");
    QDBusConnection::sessionBus().registerObject("/org/deepin/dde/Dock1", "org.deepin.dde.Dock1", proxy);

    DockDaemonAdaptor* dockDaemonAdaptor = new DockDaemonAdaptor(proxy);
    QDBusConnection::sessionBus().registerService("org.deepin.dde.daemon.Dock1");
    QDBusConnection::sessionBus().registerObject("/org/deepin/dde/daemon/Dock1", "org.deepin.dde.daemon.Dock1", proxy);
    connect(SETTINGS, &DockSettings::positionChanged, this, [this, dockDaemonAdaptor](){
        Q_EMIT positionChanged(position());
        Q_EMIT dockDaemonAdaptor->PositionChanged(position());
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());

        QMetaObject::invokeMethod(this,[this](){
            Q_EMIT onWindowGeometryChanged();
        });
    });

    connect(this, &DockPanel::frontendWindowRectChanged, dockDaemonAdaptor, &DockDaemonAdaptor::FrontendWindowRectChanged);
    connect(SETTINGS, &DockSettings::dockSizeChanged, this, &DockPanel::dockSizeChanged);
    connect(SETTINGS, &DockSettings::hideModeChanged, this, &DockPanel::hideModeChanged);
    connect(SETTINGS, &DockSettings::itemAlignmentChanged, this, &DockPanel::itemAlignmentChanged);
    connect(SETTINGS, &DockSettings::indicatorStyleChanged, this, &DockPanel::indicatorStyleChanged);

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

    DPanel::init();

    QObject::connect(this, &DApplet::rootObjectChanged, this, [this]() {
        if (rootObject()) {
            // those connections need connect after DPanel::init() which create QQuickWindow
            // xChanged yChanged not woker on wayland, so use above positionChanged instead
            // connect(window(), &QQuickWindow::xChanged, this, &DockPanel::onWindowGeometryChanged);
            // connect(window(), &QQuickWindow::yChanged, this, &DockPanel::onWindowGeometryChanged);
            connect(window(), &QQuickWindow::widthChanged, this, &DockPanel::onWindowGeometryChanged);
            connect(window(), &QQuickWindow::heightChanged, this, &DockPanel::onWindowGeometryChanged);
            QMetaObject::invokeMethod(this, &DockPanel::onWindowGeometryChanged);
        }
    });

    m_theme = static_cast<ColorTheme>(Dtk::Gui::DGuiApplicationHelper::instance()->themeType());
    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        // TODO: support get color type from wayland
        m_helper = new WaylandDockHelper(this);
    } else if (QStringLiteral("xcb") == platformName) {
        QObject::connect(Dtk::Gui::DGuiApplicationHelper::instance(), &Dtk::Gui::DGuiApplicationHelper::themeTypeChanged,
                            this, [this](){
            setColorTheme(static_cast<ColorTheme>(Dtk::Gui::DGuiApplicationHelper::instance()->themeType()));
        });
        m_helper = new X11DockHelper(this);
    }


    connect(m_helper, &DockHelper::mouseInDockAreaChanged, this, [this](){
        if (hideMode() == KeepShowing) return;
        if (m_helper->mouseInDockArea()) {
            m_hideState = Show;
            Q_EMIT hideStateChanged(m_hideState);
        } else {
            m_hideState = Hide;
            Q_EMIT hideStateChanged(m_hideState);
        }
    });

    QMetaObject::invokeMethod(this, [this, dockDaemonAdaptor](){
        Q_EMIT dockDaemonAdaptor->FrontendWindowRectChanged(frontendWindowRect());
    });

    // TODO: get launchpad status from applet not dbus
    QDBusConnection::sessionBus().connect("org.deepin.dde.Launcher1", "/org/deepin/dde/Launcher1", "org.deepin.dde.Launcher1", "VisibleChanged", this, SLOT(launcherVisibleChanged(bool)));

    return true;
}

QRect DockPanel::geometry()
{
    Q_ASSERT(window());
    return window()->geometry();
}

QRect DockPanel::frontendWindowRect()
{
    if(!window()) return QRect();

    auto ratio = window()->devicePixelRatio();
    auto screenGeometry = window()->screen()->geometry();
    auto geometry = window()->geometry();
    auto x = 0, y = 0;
    switch (position()) {
        case Top:
            x = (screenGeometry.width() - geometry.width()) / 2;
            break;
        case Bottom:
            x = (screenGeometry.width() - geometry.width()) / 2;
            y = screenGeometry.height() - geometry.height();
            break;
        case Right:
            x = screenGeometry.width() - geometry.width();
            y = (screenGeometry.height() - geometry.height()) / 2;
            break;
        case Left:
            y = screenGeometry.height() - geometry.height();
            break;
    }
    return QRect(x * ratio, y * ratio, geometry.width() * ratio, geometry.height() * ratio);
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

void DockPanel::setPosition(const Position& position)
{
    SETTINGS->setPosition(position);
}

ItemAlignment DockPanel::itemAlignment()
{
    return SETTINGS->itemAlignment();
}

void DockPanel::setItemAlignment(const ItemAlignment& alignment)
{
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
    Q_EMIT frontendWindowRectChanged(frontendWindowRect());
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
    if (hideMode() == KeepShowing || m_launcherShown)
        return Show;
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
        .arg(QStringLiteral("personalization/desktop/dock"))
        .call();
}

void DockPanel::loadDockPlugins()
{
    if(!m_compositorReady) return;

    QStringList filters;
    filters << "*.so";

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_SCALE_FACTOR", QString::number(qApp->devicePixelRatio()));
    env.insert("D_DXCB_DISABLE_OVERRIDE_HIDPI", "1");
    proc.setProcessEnvironment(env);
    QStringList execPaths;
    execPaths << qEnvironmentVariable("TRAY_LOADER_EXECUTE_PATH")
              << QString("%1/dockplugin-loader").arg(CMAKE_INSTALL_FULL_LIBEXECDIR);
    QString validExePath;
    for (const QString & execPath : execPaths) {
         if (QFile::exists(execPath)) {
             validExePath = execPath;
             break;
         }
    }
    qInfo() << "Valid Loader Execute Path:" << validExePath;
    proc.setProgram(validExePath);

    QStringList dirs;;
    const auto pluginsPath = qEnvironmentVariable("TRAY_DEBUG_PLUGIN_PATH");
    if (!pluginsPath.isEmpty())
        dirs << pluginsPath.split(QDir::listSeparator());

    if (dirs.isEmpty())
        dirs << pluginDirs;

    for (auto &pluginDir : dirs) {
        QDir dir(pluginDir);
        auto plugins = dir.entryList(filters, QDir::Files);
        foreach(QString plugin, plugins) {
            plugin = pluginDir + plugin;
            qInfo() << "pluginLoader load plugin" << plugin;
            proc.setArguments({"-p", plugin, "-platform", "wayland",});
            proc.startDetached();
        }
    }
}

void DockPanel::launcherVisibleChanged(bool visible)
{
    if (visible == m_launcherShown) return;

    m_launcherShown = visible;
    if (hideMode() != KeepShowing) {
        Q_EMIT hideStateChanged(hideState());
    }
}

void DockPanel::setMouseGrabEnabled(QQuickItem *item, bool enabled)
{
    if (!item) return;

    auto window = item->window();
    if (!window) return;

    window->setMouseGrabEnabled(enabled);
}

D_APPLET_CLASS(DockPanel)

}

#include "dockpanel.moc"
