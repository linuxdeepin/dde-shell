// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsglobal.h"
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QHash>

DS_BEGIN_NAMESPACE

// Metadata for the pinned icon only. The provider's QML view owns all content
// and operations; the framework and task manager do not model its entries.
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
    virtual QVariantList previewIcons() const { return {}; }
    virtual QUrl iconUrl() const { return {}; }
    virtual QUrl contentUrl() const { return {}; }
signals:
    void presentationChanged();
    void identityChanged(const QString &id);
    void contentLost();
};

class FolderContentProvider
{
public:
    virtual ~FolderContentProvider() = default;
    virtual QString normalizeId(const QString &id) const = 0;
    virtual FolderPresentation *create(const QString &id, QObject *parent) = 0;
};

class DS_SHARE FolderContentRegistry : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    static FolderContentRegistry *instance();
    static bool isReference(const QString &reference);
    static QString reference(const QString &source, const QString &id);
    // Rebuild an existing reference with a new provider id, keeping its source.
    QString referenceWithId(const QString &reference, const QString &id) const;
    QString normalizeReference(const QString &reference);
    FolderPresentation *create(const QString &reference, QObject *parent);
private:
    FolderContentProvider *provider(const QString &source);
    static bool decode(const QString &reference, QString *source, QString *id);
    QHash<QString, FolderContentProvider *> m_providers;
};

DS_END_NAMESPACE

#define DS_FOLDER_CONTENT_PROVIDER_IID "org.deepin.ds.FolderContentProvider/1.0"
Q_DECLARE_INTERFACE(DS_NAMESPACE::FolderContentProvider, DS_FOLDER_CONTENT_PROVIDER_IID)
