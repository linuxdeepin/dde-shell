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
#include <QApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QFile>
#include <QMenu>

#include <qjson/parser.h>

#include <dbusmenuexporter.h>

static const char *DBUS_SERVICE = "org.dbusmenu.test";
static const char *DBUS_PATH    = "/MenuBar";
static const char *USAGE        = "dbusmenubench-qtapp <path/to/menu.json>";

void createMenuItem(QMenu *menu, const QVariant &item)
{
    QVariantMap map = item.toMap();

    if (map.value("visible").toString() == "false") {
        return;
    }

    QString type = map.value("type").toString();
    if (type == "separator") {
        menu->addSeparator();
        return;
    }

    QString label = map.value("label").toString();
    QAction *action = menu->addAction(label);
    action->setEnabled(map.value("sensitive").toString() == "true");
    if (map.contains("submenu")) {
        QVariantList items = map.value("submenu").toList();
        Q_FOREACH(const QVariant &item, items) {
            QMenu *subMenu = new QMenu;
            action->setMenu(subMenu);
            createMenuItem(subMenu, item);
        }
    }
}

void initMenu(QMenu *menu, const QString &fileName)
{
    QJson::Parser parser;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not open file" << fileName;
        return;
    }

    bool ok;
    QVariant tree = parser.parse(&file, &ok);
    if (!ok) {
        qCritical() << "Could not parse json data from" << fileName;
        return;
    }

    QVariantList list = tree.toList();
    Q_FOREACH(const QVariant &item, list) {
        createMenuItem(menu, item);
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QMenu menu;

    if (argc != 2) {
        qCritical() << USAGE;
        return 1;
    }
    QString jsonFileName = argv[1];
    initMenu(&menu, jsonFileName);

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService(DBUS_SERVICE)) {
        qCritical() << "Could not register" << DBUS_SERVICE;
        return 1;
    }

    DBusMenuExporter exporter(DBUS_PATH, &menu);
    return app.exec();
}
