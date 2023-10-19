// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginloader.h"

#include "applet.h"
#include "containment.h"
#include "pluginmetadata.h"
#include "pluginfactory.h"

#include <dobject_p.h>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QPluginLoader>
#include <QStandardPaths>

DS_BEGIN_NAMESPACE;

DCORE_USE_NAMESPACE

static const QString MetaDataFileName{"metadata.json"};

Q_DECLARE_LOGGING_CATEGORY(dsLog)

class DPluginLoaderPrivate : public DObjectPrivate
{
public:
    explicit DPluginLoaderPrivate(DPluginLoader *qq)
        : DObjectPrivate(qq)
    {
        m_pluginDirs = builtinPackagePaths();
    }
    void init()
    {
        D_Q(DPluginLoader);

        for (auto item : builtinPluginPaths()) {
            q->addPluginDir(item);
        }

        const QString rootDir(QCoreApplication::applicationDirPath());

        for (auto item : m_pluginDirs) {
            const QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories;
            const QStringList nameFilters = {MetaDataFileName};

            QDirIterator it(item, nameFilters, QDir::Files, flags);
            QSet<QString> dirs;
            while (it.hasNext()) {
                it.next();

                const QString dir = it.fileInfo().absoluteDir().path();
                if (dirs.contains(dir)) {
                    continue;
                }
                dirs << dir;

                const QString metadataPath = it.fileInfo().absoluteFilePath();
                DPluginMetaData info = DPluginMetaData::fromJsonFile(metadataPath);
                if (!info.isValid())
                    continue;

                if (m_plugins.contains(info.pluginId()))
                    continue;

                m_plugins[info.pluginId()] = info;
            }
        }
    }

    QStringList builtinPackagePaths()
    {
        QStringList result;
        // 'DDE_SHELL_PACKAGE_PATH' directory.
        const auto dtkPluginPath = qgetenv("DDE_SHELL_PACKAGE_PATH");
        if (!dtkPluginPath.isEmpty())
            result.append(dtkPluginPath);

        const QString packageDir = buildingDir("packages");
        if (!packageDir.isEmpty())
            result << packageDir;

        for (auto item : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
            result << item + "/dde-shell";
        }
        qCDebug(dsLog()) << "Buildin package paths" << result;

        return result;
    }

    QStringList builtinPluginPaths()
    {
        QStringList result;
        // 'DDE_SHELL_PACKAGE_PATH' directory.
        const auto dtkPluginPath = qgetenv("DDE_SHELL_PLUGIN_PATH");
        if (!dtkPluginPath.isEmpty())
            result.append(dtkPluginPath);

        const QString pluginsDir = buildingDir("plugins");
        if (!pluginsDir.isEmpty())
            result << pluginsDir;

        result <<  DDE_SHELL_PLUGIN_INSTALL_DIR;

        qCDebug(dsLog()) << "Buildin plugin paths" << result;
        return result;
    }

    DAppletFactory *appletFactory(const DPluginMetaData &data)
    {
        DAppletFactory *factory = nullptr;
        const QString fileName = data.pluginId();
        QPluginLoader loader(fileName);
        loader.load();
        if (!loader.isLoaded()) {
            return factory;
        }

        const auto &meta = loader.metaData();

        do {
            const auto iid = meta["IID"].toString();
            if (iid.isEmpty())
                break;

            if (iid != QString(qobject_interface_iid<DAppletFactory *>()))
                break;

            if (!loader.instance()) {
                qWarning(dsLog) << "Load the plugin failed." << loader.errorString();
                break;
            }
            factory = qobject_cast<DAppletFactory *>(loader.instance());
            if (!factory) {
                qWarning(dsLog) << "The plugin isn't a DAppletFactory." << fileName;
                break;
            }
        } while (false);

        return factory;
    }

    DPluginMetaData pluginMetaData(const QString &pluginId) const
    {
        const auto it = m_plugins.constFind(pluginId);
        if (it == m_plugins.constEnd())
            return DPluginMetaData();
        return it.value();
    }

    inline QString buildingDir(const QString &subdir)
    {
        QDir dir(QCoreApplication::applicationDirPath());
        dir.cdUp();
        if (dir.exists() && dir.exists(subdir))
            return dir.absoluteFilePath(subdir);

        return QString();
    }

    QStringList m_pluginDirs;
    QMap<QString, DPluginMetaData> m_plugins;

    D_DECLARE_PUBLIC(DPluginLoader)
};

DPluginLoader::DPluginLoader()
    : DObject(*new DPluginLoaderPrivate(this))
{
    D_D(DPluginLoader);
    d->init();
}

DPluginLoader::~DPluginLoader()
{

}

DPluginLoader *DPluginLoader::instance()
{
    static DPluginLoader g_instance;
    return &g_instance;
}

QList<DPluginMetaData> DPluginLoader::plugins() const
{
    D_DC(DPluginLoader);
    return d->m_plugins.values();
}

void DPluginLoader::addPackageDir(const QString &dir)
{
    D_D(DPluginLoader);
    d->m_pluginDirs.prepend(dir);
    d->init();
}

void DPluginLoader::addPluginDir(const QString &dir)
{
    if (QCoreApplication::libraryPaths().contains(dir))
        return;
    QCoreApplication::addLibraryPath(dir);
}

DApplet *DPluginLoader::loadApplet(const QString &pluginId)
{
    D_D(DPluginLoader);
    DPluginMetaData metaData = d->pluginMetaData(pluginId);
    if (!metaData.isValid())
        return nullptr;

    DApplet *applet = nullptr;
    if (auto factory = d->appletFactory(metaData)) {
        qCDebug(dsLog()) << "Loading applet by factory" << pluginId;
        applet = factory->create();
    }
    if (!applet) {
        if (metaData.value("ContainmentType").isValid()) {
            applet = new DContainment();
        }
    }

    if (!applet) {
        applet = new DApplet();
    }
    if (applet) {
        applet->setMetaData(metaData);
    }
    return applet;
}

QList<DPluginMetaData> DPluginLoader::childrenPlugin(const QString &pluginId) const
{
    D_DC(DPluginLoader);
    DPluginMetaData metaData = d->pluginMetaData(pluginId);
    if (!metaData.isValid())
        return {};

    const DPluginMetaData target(metaData);
    QList<DPluginMetaData> result;
    for (auto md : d->m_plugins.values()) {
        const QString parentId(md.value("Parent").toString());
        if (parentId == target.pluginId()) {
            result << md;
        }
    }
    return result;
}

DPluginMetaData DPluginLoader::parentPlugin(const QString &pluginId) const
{
    D_DC(DPluginLoader);
    DPluginMetaData metaData = d->pluginMetaData(pluginId);
    if (!metaData.isValid())
        return DPluginMetaData();

    const QString parentId(metaData.value("Parent").toString());
    return d->pluginMetaData(parentId);
}

DS_END_NAMESPACE
