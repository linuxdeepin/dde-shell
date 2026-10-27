// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariant>

namespace dock {

// Optional menu extensions owned by folder-source plugins. Providers remain
// owned by their plugin and expose the two invokables documented in docs/.
class FolderMenuRegistry : public QObject
{
    Q_OBJECT
public:
    explicit FolderMenuRegistry(QObject *parent = nullptr);

    bool registerProvider(const QString &folderType, QObject *provider);
    void unregisterProvider(const QString &folderType, QObject *provider);
    static QVariantList standardItems(const QString &folderName);
    QVariantList menuItems(const QString &folderType, const QString &folderId, const QString &folderName) const;
    bool trigger(const QString &folderType, const QString &folderId, const QString &actionId) const;

private:
    QVariantList customItems(QObject *provider, const QString &folderId) const;
    QHash<QString, QPointer<QObject>> m_providers;
};

}
