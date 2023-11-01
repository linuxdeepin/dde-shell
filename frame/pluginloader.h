// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "pluginmetadata.h"
#include <QObject>
#include <DObject>

DS_BEGIN_NAMESPACE

class DApplet;

/**
 * @brief 插件加载，创建
 */
class DPluginLoaderPrivate;
class DS_SHARE DPluginLoader : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    D_DECLARE_PRIVATE(DPluginLoader)
public:
    explicit DPluginLoader();
    virtual ~DPluginLoader() override;

    static DPluginLoader *instance();
    QList<DPluginMetaData> plugins() const;
    QList<DPluginMetaData> rootPlugins() const;
    void addPackageDir(const QString &dir);
    void addPluginDir(const QString &dir);

    DApplet *loadApplet(const QString &pluginId);

    QList<DPluginMetaData> childrenPlugin(const QString &pluginId) const;
    DPluginMetaData parentPlugin(const QString &pluginId) const;
};

DS_END_NAMESPACE
