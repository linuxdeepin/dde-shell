/* This file is part of the dbusmenu-qt library
   Copyright 2009 Canonical
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
#ifndef DBUSMENUEXPORTERTEST_H
#define DBUSMENUEXPORTERTEST_H

#define QT_GUI_LIB
#include <QtGui>

// Qt
#include <QObject>

// Local

class DBusMenuExporterTest : public QObject
{
Q_OBJECT
private Q_SLOTS:
    void testGetSomeProperties();
    void testGetSomeProperties_data();
    void testGetAllProperties();
    void testGetNonExistentProperty();
    void testClickedEvent();
    void testSubMenu();
    void testDynamicSubMenu();
    void testRadioItems();
    void testNonExclusiveActionGroup();
    void testClickDeletedAction();
    void testDeleteExporterBeforeMenu();
    void testUpdateAndDeleteSubMenu();
    void testMenuShortcut();
    void testGetGroupProperties();
    void testActivateAction();
    void testTrackActionsOnlyOnce();
    void testHonorDontShowIconsInMenusAttribute();
    void testDBusMenuObjectIsDeletedWhenExporterIsDeleted();
    void testSeparatorCollapsing_data();
    void testSeparatorCollapsing();
    void testSetStatus();
    void testGetIconDataProperty();

    void init();
    void cleanup();
};

#endif /* DBUSMENUEXPORTERTEST_H */
