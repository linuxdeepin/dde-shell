// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>

namespace dock {

QStringList resolveDockedElements(const QStringList &elements);
QStringList defaultFolderDockedElements();
bool shouldMigrateDefaultDockFolders(const QStringList &elements);
QStringList mergedWithDefaultDockFolders(const QStringList &elements);

}
