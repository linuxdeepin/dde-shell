// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "appletitem.h"
#include "dockapplet.h"
#include "pluginfactory.h"
#include "dbusdockadaptors.h"
#include "docktraywindow.h"

#include <QApplication>
#include <QDebug>
#include <QQuickWindow>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QWindow>

DS_BEGIN_NAMESPACE
    namespace dock {

                   DockApplet::DockApplet(QObject *parent)
                   : DApplet(parent)
                   , m_window(new DockTrayWindow)
                   , m_dockWidth(0)
                   , m_dockHeight(0)
                   , m_dockAdapter(nullptr)
                   {

                   }

                   void DockApplet::setDockWidth(int width)
                   {
                    if (m_dockWidth != width) {
                                              m_dockWidth = width;
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
        Q_EMIT dockHeightChanged(height);
    }
}

int DockApplet::dockHeight() const
{
    return m_dockHeight;
}

void DockApplet::setPanelPosition(int x, int y) const
{
    m_window->move(x, y);
}

void DockApplet::setDockPosition(int pos) const
{
    m_window->setPositon(Dock::Position(pos));
}

void DockApplet::setPanelSize(int size) const
{
    m_window->setDockSize(size);
}

void DockApplet::setDisplayMode(int displayMode) const
{
    m_window->setDisplayMode(Dock::DisplayMode(displayMode));
}

void DockApplet::initDock()
{
    static bool init = false;
    if (init) return;
    init = !init;

    auto appletItem = qobject_cast<DAppletItem *>(rootObject());
    if (appletItem) {
        m_window->winId();
        m_window->windowHandle()->setParent(appletItem->window());
        m_window->setFixedSize(m_window->suitableSize());
        m_window->show();

        // Make drop event of qml can v                                                                                                                                                                                                                                                                                                     delivery to qwidget
        appletItem->window()->setObjectName("_ds_drop_delivery_internal");
        appletItem->window()->installEventFilter(m_window);

        QTranslator *tl = new QTranslator(this);
        tl->load(QString("/usr/share/dde-dock/tmp/translations/dde-dock_%1.qm").arg(QLocale().name()));
        qApp->installTranslator(tl);

        connect(m_window, &DockTrayWindow::sizeChanged, this, [ = ] (const QSize &size) {
            setDockWidth(size.width());
            setDockHeight(size.height());
        });

        // manage dbus data
        m_dockAdapter = new OldDBusDock;
    }
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

DS_END_NAMESPACE

#include "dockapplet.moc"
