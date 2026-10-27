// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "foldercontent.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QRegularExpression>

DS_BEGIN_NAMESPACE

FolderContentRegistry *FolderContentRegistry::instance()
{
    static auto registry = new FolderContentRegistry(qApp);
    return registry;
}

QString FolderContentRegistry::reference(const QString &source, const QString &id)
{
    return QStringLiteral("folder/%1/%2").arg(source, QString::fromLatin1(id.toUtf8().toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
}

bool FolderContentRegistry::decode(const QString &value, QString *source, QString *id)
{
    const auto parts = value.split('/');
    static const QRegularExpression sourcePattern(QStringLiteral("^[a-zA-Z0-9][a-zA-Z0-9_.-]*$"));
    if (parts.size() != 3 || parts[0] != "folder" || !sourcePattern.match(parts[1]).hasMatch())
        return false;
    const auto bytes = QByteArray::fromBase64(parts[2].toLatin1(),
        QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    *source = parts[1];
    *id = QString::fromUtf8(bytes);
    return !id->isEmpty() && reference(*source, *id) == value;
}

bool FolderContentRegistry::isReference(const QString &value)
{
    QString source, id;
    return decode(value, &source, &id);
}

QString FolderContentRegistry::referenceWithId(const QString &value, const QString &id) const
{
    QString source, current;
    if (id.isEmpty() || !decode(value, &source, &current)) return {};
    return reference(source, id);
}

FolderContentProvider *FolderContentRegistry::provider(const QString &source)
{
    if (m_providers.contains(source))
        return m_providers.value(source);

    // Content providers live in the regular shell plugin directory and follow
    // the same file naming as every other shell plugin:
    // org.deepin.ds.folderprovider.<source>.so
    const auto fileName = QStringLiteral("org.deepin.ds.folderprovider.%1.so").arg(source);
    QStringList dirs {
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../plugins"),
        QStringLiteral(DDE_SHELL_PLUGIN_INSTALL_DIR)
    };
    const auto runtimePaths = qEnvironmentVariable("DDE_SHELL_PLUGIN_PATH");
    if (!runtimePaths.isEmpty())
        dirs.append(runtimePaths.split(QDir::listSeparator(), Qt::SkipEmptyParts));
    dirs.append(QCoreApplication::libraryPaths());

    for (const auto &dir : dirs) {
        const auto path = QDir(dir).filePath(fileName);
        if (!QFileInfo::exists(path))
            continue;
        auto loader = new QPluginLoader(path, this);
        auto object = loader->instance();
        auto contentProvider = qobject_cast<FolderContentProvider *>(object);
        if (contentProvider) {
            m_providers.insert(source, contentProvider);
            return contentProvider;
        }
        qWarning() << "Failed to load folder content provider:" << path
                   << (object ? QStringLiteral("FolderContentProvider interface mismatch") : loader->errorString());
        delete loader;
    }
    qWarning() << "Folder content provider unavailable:" << fileName << "search paths:" << dirs;
    return nullptr;
}

QString FolderContentRegistry::normalizeReference(const QString &value)
{
    QString source, id;
    if (!decode(value, &source, &id)) return {};
    auto factory = provider(source);
    const auto normalized = factory ? factory->normalizeId(id) : QString();
    return normalized.isEmpty() ? QString() : reference(source, normalized);
}

FolderPresentation *FolderContentRegistry::create(const QString &value, QObject *parent)
{
    QString source, id;
    if (!decode(value, &source, &id)) return nullptr;
    if (auto factory = provider(source))
        return factory->create(id, parent);
    // Keep pins when a provider is temporarily unavailable.
    return new FolderPresentation(parent);
}

DS_END_NAMESPACE
