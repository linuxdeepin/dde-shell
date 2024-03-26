/* This file is part of the dbusmenu-qt library
   Copyright 2010 Canonical
   Author: Aurelien Gateau <aurelien.gateau@canonical.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License (LGPL) as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include <slowmenu.moc>

#include <dbusmenuexporter.h>

#include <QtDBus>
#include <QtGui>
#include <QApplication>
#include <QElapsedTimer>

static const char *TEST_SERVICE = "org.kde.dbusmenu-qt-test";
static const char *TEST_OBJECT_PATH = "/TestMenuBar";

SlowMenu::SlowMenu()
: QMenu()
{
    connect(this, SIGNAL(aboutToShow()), SLOT(slotAboutToShow()));
}

void SlowMenu::slotAboutToShow()
{
    qDebug() << __FUNCTION__ << "Entering";
    QElapsedTimer time;
    time.start();
    while (time.elapsed() < 2000) {
        qApp->processEvents();
    }
    qDebug() << __FUNCTION__ << "Leaving";
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QDBusConnection::sessionBus().registerService(TEST_SERVICE);
    SlowMenu* inputMenu = new SlowMenu;
    inputMenu->addAction("Test");
    DBusMenuExporter exporter(TEST_OBJECT_PATH, inputMenu);
    qDebug() << "Looping";
    return app.exec();
}
