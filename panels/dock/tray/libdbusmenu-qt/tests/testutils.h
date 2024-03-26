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
#ifndef TESTUTILS_H
#define TESTUTILS_H

// Local
#include <debug_p.h>
#include <dbusmenutypes_p.h>

// Qt
#include <QObject>
#include <QMenu>
#include <QVariant>

class ManualSignalSpy : public QObject, public QList<QVariantList>
{
    Q_OBJECT
public Q_SLOTS:
    void receiveCall(int value)
    {
        append(QVariantList() << value);
    }

    void receiveCall(uint v1, int v2)
    {
        append(QVariantList() << v1 << v2);
    }

    void receiveCall(int v1, uint v2)
    {
        append(QVariantList() << v1 << v2);
    }

    void receiveCall(DBusMenuItemList itemList, DBusMenuItemKeysList removedPropsList)
    {
        QVariantList propsIds;
        Q_FOREACH(DBusMenuItem item, itemList) {
            propsIds << item.id;
        }
        QVariantList removedPropsIds;
        Q_FOREACH(DBusMenuItemKeys props, removedPropsList) {
            removedPropsIds << props.id;
        }

        QVariantList args;
        args.push_back(propsIds);
        args.push_back(removedPropsIds);
        append(args);
    }

    void receiveCall(const QString& service, const QVariantMap& modifiedProperties, const QStringList& newProperties)
    {
        QVariantList args;
        args.push_back(service);
        args.push_back(modifiedProperties);
        args.push_back(newProperties);
        append(args);
    }
};

class MenuFiller : public QObject
{
    Q_OBJECT
public:
    MenuFiller(QMenu *menu)
    : m_menu(menu)
    {
        connect(m_menu, SIGNAL(aboutToShow()), SLOT(fillMenu()));
    }

    void addAction(QAction *action)
    {
        m_actions << action;
    }

public Q_SLOTS:
    void fillMenu()
    {
        while (!m_actions.isEmpty()) {
            m_menu->addAction(m_actions.takeFirst());
        }
    }

private:
    QMenu *m_menu;
    QList<QAction *> m_actions;
};

void waitForDeferredDeletes();

#endif /* TESTUTILS_H */
