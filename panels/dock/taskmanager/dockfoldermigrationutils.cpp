// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockfoldermigrationutils.h"

#include <QDir>
#include <QSet>
#include <QStandardPaths>

namespace dock {
namespace {
static const QString DOWNLOADS_PLACEHOLDER = QStringLiteral("folder/$DOWNLOADS");
static const QString APPLICATIONS_FOLDER_DOCK_ELEMENT = QStringLiteral("folder//usr/share/applications");
static const QString FILE_MANAGER_DOCK_ELEMENT = QStringLiteral("desktop/dde-file-manager");

static QString resolveDockedElement(const QString &element)
{
    if (element != DOWNLOADS_PLACEHOLDER) {
        return element;
    }

    const QString downloadsPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    if (downloadsPath.isEmpty()) {
        return {};
    }

    return QStringLiteral("folder/%1").arg(downloadsPath);
}

static QSet<QString> asSet(const QStringList &elements)
{
    QSet<QString> elementSet;
    for (const QString &element : elements) {
        elementSet.insert(element);
    }

    return elementSet;
}
}

QStringList resolveDockedElements(const QStringList &elements)
{
    QStringList resolvedElements;

    for (const QString &element : elements) {
        const QString resolvedElement = resolveDockedElement(element);
        if (resolvedElement.isEmpty() || resolvedElements.contains(resolvedElement)) {
            continue;
        }

        resolvedElements.append(resolvedElement);
    }

    return resolvedElements;
}

QStringList defaultFolderDockedElements()
{
    return resolveDockedElements({DOWNLOADS_PLACEHOLDER, APPLICATIONS_FOLDER_DOCK_ELEMENT});
}

bool shouldMigrateDefaultDockFolders(const QStringList &elements)
{
    if (!elements.contains(FILE_MANAGER_DOCK_ELEMENT)) {
        return false;
    }

    const QSet<QString> expectedFolderElements = asSet(defaultFolderDockedElements());
    bool hasDesktopElement = false;
    for (const QString &element : elements) {
        if (element.startsWith(QStringLiteral("desktop/"))) {
            hasDesktopElement = true;
            continue;
        }

        // Only inject the new default folders when the existing folder entries are
        // also defaults. Unknown folder/group items indicate user customization.
        if (element.startsWith(QStringLiteral("folder/")) && expectedFolderElements.contains(element)) {
            continue;
        }

        return false;
    }

    return hasDesktopElement;
}

QStringList mergedWithDefaultDockFolders(const QStringList &elements)
{
    QStringList mergedElements = elements;
    int insertIndex = mergedElements.indexOf(FILE_MANAGER_DOCK_ELEMENT);
    if (insertIndex < 0) {
        insertIndex = -1;
    }

    for (const QString &folderElement : defaultFolderDockedElements()) {
        const int existingIndex = mergedElements.indexOf(folderElement);
        if (existingIndex >= 0) {
            insertIndex = existingIndex;
            continue;
        }

        mergedElements.insert(++insertIndex, folderElement);
    }

    return resolveDockedElements(mergedElements);
}

}
