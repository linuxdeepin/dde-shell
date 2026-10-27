// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "foldercontent.h"

class TestPresentation : public dock::FolderPresentation
{
    Q_OBJECT
public:
    using dock::FolderPresentation::FolderPresentation;
    QString title() const override { return m_title; }
    QVariantList previewIcons() const override { return {QUrl("image://icon/folder")}; }
    Q_INVOKABLE void rename(const QString &title) { m_title = title; emit presentationChanged(); }
    // Emulate a provider that resolved a new id for the same item, e.g. after
    // the pinned directory was renamed on disk.
    Q_INVOKABLE void rebind(const QString &id) { emit identityChanged(id); }
    // Emulate a provider whose pinned directory was deleted on disk.
    Q_INVOKABLE void lose() { emit contentLost(); }
private:
    QString m_title = "Test folder";
};

class TestFolderProvider : public QObject, public dock::FolderContentProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DS_FOLDER_CONTENT_PROVIDER_IID)
    Q_INTERFACES(dock::FolderContentProvider)
public:
    QString normalizeId(const QString &id) const override
    {
        if (id == "test://folder/") return QStringLiteral("test://folder");
        return id == "test://folder" || id == "test://folder-renamed" ? id : QString();
    }
    dock::FolderPresentation *create(const QString &, QObject *parent) override { return new TestPresentation(parent); }
};
#include "testfolderprovider.moc"
