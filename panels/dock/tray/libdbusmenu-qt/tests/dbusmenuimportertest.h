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
#ifndef DBUSMENUIMPORTERTEST_H
#define DBUSMENUIMPORTERTEST_H

#define QT_GUI_LIB
#include <QtGui>

// Qt
#include <QObject>

// Local

class DBusMenuImporterTest : public QObject
{
Q_OBJECT
private Q_SLOTS:
    void cleanup();
    void testStandardItem();
    void testAddingNewItem();
    void testShortcut();
    void testDeletingImporterWhileWaitingForAboutToShow();
    void testDynamicMenu();
    void testActionActivationRequested();
    void testActionsAreDeletedWhenImporterIs();
    void testIconData();
    void testInvisibleItem();
    void testDisabledItem();

    void initTestCase();
};

#endif /* DBUSMENUIMPORTERTEST_H */
