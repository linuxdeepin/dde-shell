// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "appletitem.h"
#include "dockapplet.h"
#include "pluginfactory.h"
#include "dbusdockadaptors.h"
#include "docktraywindow.h"
#include "quickproxywidget.h"

#include <QApplication>
#include <QDebug>
#include <QQuickWindow>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QWindow>

namespace dock {

DockApplet::DockApplet(QObject *parent)
    : DApplet(parent)
    , m_dockAdapter(nullptr)
    , m_widgetProxy(nullptr)
    , m_dockWidth(0)
    , m_dockHeight(0)
{
}

DockApplet::~DockApplet()
{
    if (m_window)
        m_window->deleteLater();
}

void DockApplet::setDockWidth(int width)
{
    if (m_dockWidth != width) {
        m_dockWidth = width;
        if (m_widgetProxy) {
            m_widgetProxy->setImplicitWidth(width);
        }
        Q_EMIT dockWidthChanged(width);
    }
}

int DockApplet::dockWidth() const
{
    return m_dockWidth;
}

void DockApplet::setDockHeight(int height)
{
    if (m_dockHeight != height) {
        m_dockHeight = height;
        if (m_widgetProxy) {
            m_widgetProxy->setImplicitHeight(height);
        }
        Q_EMIT dockHeightChanged(height);
    }
}

int DockApplet::dockHeight() const
{
    return m_dockHeight;
}

void DockApplet::setDockPosition(int pos) const
{
    if (m_window)
        m_window->setPositon(Dock::Position(pos));
}

void DockApplet::setPanelSize(int size) const
{
    if (m_window)
        m_window->setDockSize(size);
}

void DockApplet::setDisplayMode(int displayMode) const
{
    if (m_window)
        m_window->setDisplayMode(Dock::DisplayMode(displayMode));
}

void DockApplet::initDock()
{
    static bool init = false;
    if (init) return;
    init = !init;

    m_window = new DockTrayWindow;

    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::UseInactiveColorGroup, false);

    auto appletItem = qobject_cast<DS_NAMESPACE::DAppletItem *>(rootObject());
    if (appletItem) {
        m_widgetProxy = rootObject()->findChild<QuickProxyWidget*>();
        if (!m_widgetProxy) {
            qWarning() << "failed to insert dock widget to qml";
            return;
        }
        m_widgetProxy->setWidget(m_window);

        connect(m_window, &DockTrayWindow::sizeChanged, this, [ = ] {
            setDockWidth(m_window->width());
            setDockHeight(m_window->height());
        });

        // manage dbus data
        m_dockAdapter = new OldDBusDock;
    }
}

void DockApplet::collapseExpandedPanel()
{
    if (m_window)
        m_window->collapseExpandedPanel();
}

bool DockApplet::init()
{
    DApplet::init();

    QTranslator *tl = new QTranslator(this);
    auto loaded = tl->load(QString("/usr/share/dde-dock/tmp/translations/dde-dock_%1.qm").arg(QLocale().name()));
    if(!loaded) {
        qWarning() << "Faield to load translator of dock";
    } else {
        qApp->installTranslator(tl);
    }

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    return true;
}

bool DockApplet::load()
{
    qmlRegisterType<QuickProxyWidget>("WidgetProxy", 1, 0, "WidgetProxy");

    return true;
}

DockItemInfos DockApplet::plugins()
{
    if (m_dockAdapter) {
        return m_dockAdapter->plugins();
    }

    return DockItemInfos();
}

void DockApplet::setItemOnDock(const QString settingKey, const QString &itemKey, bool visible)
{
    if (m_dockAdapter) {
        return m_dockAdapter->setItemOnDock(settingKey, itemKey, visible);
    }
}

D_APPLET_CLASS(DockApplet)
}


#include "dockapplet.moc"
