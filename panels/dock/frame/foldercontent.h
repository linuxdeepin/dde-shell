// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsglobal.h"
#include <DObject>
#include <QObject>
#include <QUrl>
#include <QVariantList>

namespace dock {

// Metadata for the pinned icon only. The provider's QML view owns all content
// and operations; the framework and task manager do not model its entries.
// Create, access and destroy presentations on the application's main thread.
// Deliver asynchronous updates to that thread before emitting signals.
class DS_SHARE FolderPresentation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY presentationChanged)
    Q_PROPERTY(QVariantList previewIcons READ previewIcons NOTIFY presentationChanged)
    Q_PROPERTY(QUrl iconUrl READ iconUrl CONSTANT)
    Q_PROPERTY(QUrl contentUrl READ contentUrl CONSTANT)
public:
    using QObject::QObject;
    virtual QString title() const { return tr("Folder"); }
    // Each entry holds a QUrl usable as a QML Image source, in display order.
    // An empty list means no preview icons are available.
    virtual QVariantList previewIcons() const { return {}; }
    // URLs remain constant for the lifetime of this presentation. An empty
    // iconUrl requests the default folder icon; an empty contentUrl means no view.
    virtual QUrl iconUrl() const { return {}; }
    virtual QUrl contentUrl() const { return {}; }
signals:
    // Notify changes to title or previewIcons, including asynchronous updates.
    void presentationChanged();
    // The new, non-empty provider id, not an encoded folder reference.
    void identityChanged(const QString &id);
    // The content is permanently gone; do not emit for temporary unavailability.
    void contentLost();
};

// Implemented by a plugin QObject owned by QPluginLoader; callers must not
// delete the provider. Methods run on the application's main thread and must
// return promptly, without blocking network requests or expensive content scans.
//
// The v1 virtual interface is frozen. Add optional capabilities as separate
// interfaces with their own IIDs on the same QObject, queried via qobject_cast.
// Incompatible changes require a new interface IID.
class FolderContentProvider
{
public:
    virtual ~FolderContentProvider() = default;
    // Return a stable, non-empty canonical id, or an empty string for an invalid
    // id. Normalizing a canonical id must return it unchanged. Temporary content
    // unavailability must not invalidate an otherwise valid id.
    virtual QString normalizeId(const QString &id) const = 0;
    // Return a new presentation parented to parent, or nullptr if creation fails.
    // If parent is null, the caller owns the result. Temporary unavailability
    // should be represented by a presentation that updates asynchronously.
    virtual FolderPresentation *create(const QString &id, QObject *parent) = 0;
};

class FolderContentRegistryPrivate;

// Use instance() and instance methods on the application's main thread, after
// QCoreApplication has been constructed. Providers are loaded lazily and cached
// for this registry's lifetime; changing search paths does not replace them.
class DS_SHARE FolderContentRegistry : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    D_DECLARE_PRIVATE(FolderContentRegistry)
public:
    explicit FolderContentRegistry(QObject *parent = nullptr);
    ~FolderContentRegistry() override;

    static FolderContentRegistry *instance();
    static bool isReference(const QString &reference);
    static QString reference(const QString &source, const QString &id);
    // Rebuild an existing reference with a new provider id, keeping its source.
    QString referenceWithId(const QString &reference, const QString &id) const;
    // Return empty for malformed references or ids rejected by their provider.
    // Keep a well-formed reference unchanged when its provider is unavailable.
    // A non-empty result does not imply that the content is currently available.
    QString normalizeReference(const QString &reference);
    // Return a presentation owned by parent (or the caller if parent is null).
    // An unavailable provider produces a default placeholder. Return nullptr for
    // malformed references or if an available provider cannot create the result.
    FolderPresentation *create(const QString &reference, QObject *parent);
private:
    FolderContentProvider *provider(const QString &source);
    static bool decode(const QString &reference, QString *source, QString *id);
};

} // namespace dock

#define DS_FOLDER_CONTENT_PROVIDER_IID "org.deepin.ds.FolderContentProvider/1.0"
Q_DECLARE_INTERFACE(dock::FolderContentProvider, DS_FOLDER_CONTENT_PROVIDER_IID)
