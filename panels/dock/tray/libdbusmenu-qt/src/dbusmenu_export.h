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
#ifndef DBUSMENU_EXPORT_H
#define DBUSMENU_EXPORT_H

// Include this file from here to make transition from version 0.3.5 and
// earlier easier (no need to conditionally include a file)
// #include "dbusmenu_version.h"

// Qt
#include <QtCore/QtGlobal>

#ifdef dbusmenu_qt_EXPORTS
#define DBUSMENU_EXPORT Q_DECL_EXPORT
#else
#define DBUSMENU_EXPORT Q_DECL_IMPORT
#endif

#endif /* DBUSMENU_EXPORT_H */
