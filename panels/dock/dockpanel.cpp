// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "dsglobal.h"
#include "constants.h"
#include "dockpanel.h"
#include "dockadaptor.h"
#include "environments.h"
#include "docksettings.h"
#include "pluginfactory.h"

// for old api compatible
#include "dockdbusproxy.h"
#include "dockfrontadaptor.h"
#include "dockdaemonadaptor.h"

#include <QQuickWindow>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(dockLog, "dde.shell.dock")

DS_BEGIN_NAMESPACE
namespace dock {

DockPanel::DockPanel(QObject * parent)
    : DPanel(parent)
    , m_theme(ColorTheme::Light)
    , m_compositorReady(false)
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
    connect(DockSettings::instance(), &DockSettings::positionChanged, this, [this](){
        Q_EMIT positionChanged(position());
        QMetaObject::invokeMethod(this,[this](){
            Q_EMIT onWindowGeometryChanged();
        });
    });
    connect(this, &DockPanel::displayModeChanged, this, &DockPanel::dockSizeChanged);
    connect(DockSettings::instance(), &DockSettings::windowSizeFashionChanged, this, &DockPanel::dockSizeChanged);
    connect(DockSettings::instance(), &DockSettings::windowSizeEfficientChanged, this, &DockPanel::dockSizeChanged);
    connect(DockSettings::instance(), &DockSettings::displayModeChanged, this, &DockPanel::displayModeChanged);
    connect(DockSettings::instance(), &DockSettings::hideModeChanged, this, &DockPanel::hideModeChanged);
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

    return true;
}

QRect DockPanel::geometry()
{
    Q_ASSERT(window());
    return window()->geometry();
}

QRect DockPanel::frontendWindowRect()
{
    Q_ASSERT(window());
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

Position DockPanel::position()
{
    return DockSettings::instance()->position();
}

void DockPanel::setPosition(Position position)
{
    DockSettings::instance()->setPosition(position);
}

HideMode DockPanel::hideMode()
{
    return DockSettings::instance()->hideMode();
}

void DockPanel::setHideMode(HideMode mode)
{
    DockSettings::instance()->setHideMode(mode);
}

DisplayMode DockPanel::displayMode()
{
    return DockSettings::instance()->displayMode();
}

void DockPanel::setDisplayMode(DisplayMode mode)
{
    DockSettings::instance()->setDisplayMode(mode);
}

ColorTheme DockPanel::colorTheme()
{
    return m_theme;
}

void DockPanel::setColorTheme(ColorTheme theme)
{
    if (theme == m_theme)
        return;
    m_theme = theme;
    Q_EMIT this->colorThemeChanged(theme);
}

uint DockPanel::dockSize()
{
    switch (displayMode()) {
    case Fashion:
        return DockSettings::instance()->windowSizeFashion();
    case Efficient:
        return DockSettings::instance()->windowSizeEfficient();
    }
    qCWarning(dockLog()) << "unknown display mode return MIN_DOCK_SIZE";
    return MIN_DOCK_SIZE;
}

void DockPanel::setDockSize(uint size)
{
    switch (displayMode()) {
        case Fashion: {
            DockSettings::instance()->setWindowSizeFashion(size);
            return;
        }
        case Efficient: {
            DockSettings::instance()->setWindowSizeEfficient(size);
        }
    }
    qCWarning(dockLog()) << "unknown display mode not set dock size to dock settings";
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
// TODO: implement this function
    return HideState::Unknown;
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
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

void DockPanel::loadDockPlugins()
{
    if(!m_compositorReady) return;

    QStringList filters;
    filters << "*.so";

    for (auto pluginDir : pluginDirs) {
        QDir dir(pluginDir);
        auto plugins = dir.entryList(filters, QDir::Files);
        foreach(QString plugin, plugins) {
            plugin = pluginDir + plugin;
#ifdef QT_DEBUG
            QProcess::startDetached(QString("%1/../panels/dock/dockplugin/loader/dockplugin-loader").arg(qApp->applicationDirPath()), {"-p", plugin, "-platform", "wayland",});
#else
            QProcess::startDetached(QString("%1/%2/dockplugin-loader").arg(CMAKE_INSTALL_PREFIX).arg(CMAKE_INSTALL_LIBEXECDIR), {"-p", plugin, "-platform", "wayland"});
#endif
        }
    }
}

D_APPLET_CLASS(DockPanel)

}

DS_END_NAMESPACE
#include "dockpanel.moc"
