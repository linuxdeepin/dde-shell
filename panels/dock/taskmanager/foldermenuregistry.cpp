// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "foldermenuregistry.h"

#include <QCoreApplication>
#include <QMetaMethod>
#include <QSet>

namespace dock {
namespace {
const QString OpenFolder = QStringLiteral("open-folder");
const QString UndockFolder = QStringLiteral("undock-folder");
}

FolderMenuRegistry::FolderMenuRegistry(QObject *parent) : QObject(parent) {}

bool FolderMenuRegistry::registerProvider(const QString &folderType, QObject *provider)
{
    if (folderType.trimmed().isEmpty() || !provider || provider->thread() != thread()) return false;
    const auto meta = provider->metaObject();
    const int actions = meta->indexOfMethod("folderMenuActions(QString)");
    const int trigger = meta->indexOfMethod("triggerFolderMenuAction(QString,QString)");
    if (actions < 0 || trigger < 0 || meta->method(actions).returnType() != QMetaType::QVariantList
        || meta->method(trigger).returnType() != QMetaType::Void) return false;
    // A different plugin may not silently replace a registered source.
    const auto existing = m_providers.value(folderType);
    if (existing && existing != provider) return false;
    m_providers.insert(folderType, provider);
    return true;
}

void FolderMenuRegistry::unregisterProvider(const QString &folderType, QObject *provider)
{
    if (provider && m_providers.value(folderType) == provider) m_providers.remove(folderType);
}

QVariantList FolderMenuRegistry::standardItems(const QString &folderName)
{
    return {QVariantMap{{"id", OpenFolder}, {"name", folderName}},
            QVariantMap{{"id", UndockFolder}, {"name", QCoreApplication::translate("FolderContextMenu", "Undock")}}};
}

QVariantList FolderMenuRegistry::customItems(QObject *provider, const QString &folderId) const
{
    QVariantList supplied;
    if (!provider || folderId.isEmpty()
        || !QMetaObject::invokeMethod(provider, "folderMenuActions", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariantList, supplied), Q_ARG(QString, folderId))) return {};
    QVariantList items;
    QSet<QString> used{OpenFolder, UndockFolder};
    for (const auto &value : supplied) {
        const auto item = value.toMap();
        const QString id = item.value("id").toString();
        const QString name = item.value("name").toString();
        if (id.trimmed().isEmpty() || name.trimmed().isEmpty() || used.contains(id)
            || !item.value("visible", true).toBool()) continue;
        used.insert(id);
        const bool checkable = item.value("checkable", false).toBool();
        items.append(QVariantMap{{"id", id}, {"name", name},
                                 {"enabled", item.value("enabled", true).toBool()},
                                 {"checkable", checkable},
                                 {"checked", checkable && item.value("checked", false).toBool()}});
    }
    return items;
}

QVariantList FolderMenuRegistry::menuItems(const QString &folderType, const QString &folderId, const QString &folderName) const
{
    auto items = standardItems(folderName);
    items.append(customItems(m_providers.value(folderType), folderId));
    return items;
}

bool FolderMenuRegistry::trigger(const QString &folderType, const QString &folderId, const QString &actionId) const
{
    const QPointer<QObject> provider = m_providers.value(folderType);
    if (!provider || actionId == OpenFolder || actionId == UndockFolder) return false;
    // Revalidate at click time: a plugin may unload or disable an action while
    // its native menu is open. Never dispatch arbitrary or stale action IDs.
    const auto items = customItems(provider, folderId);
    if (!provider || m_providers.value(folderType) != provider) return false;
    for (const auto &value : items) {
        const auto item = value.toMap();
        if (item.value("id").toString() != actionId || !item.value("enabled").toBool()) continue;
        return QMetaObject::invokeMethod(provider, "triggerFolderMenuAction", Qt::DirectConnection,
                                         Q_ARG(QString, folderId), Q_ARG(QString, actionId));
    }
    return false;
}
}
