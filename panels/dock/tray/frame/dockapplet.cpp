// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "appletitem.h"
#include "dockapplet.h"
#include "pluginfactory.h"

#include "docktraywindow.h"

#include <QWindow>
#include <QQuickWindow>
#include <QDebug>
#include <QTimer>

DS_BEGIN_NAMESPACE
    namespace dock {

                   DockApplet::DockApplet(QObject *parent)
                   : DApplet(parent)
                   , m_window(new DockTrayWindow)
                   , m_dockWidth(0)
                   , m_dockHeight(0)
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

        connect(m_window, &DockTrayWindow::sizeChanged, this, [ = ] (const QSize &size) {
            setDockWidth(size.width());
            setDockHeight(size.height());
        });
    }
}

D_APPLET_CLASS(DockApplet)
}

DS_END_NAMESPACE

#include "dockapplet.moc"
