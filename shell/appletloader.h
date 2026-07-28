// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <DObject>
#include <QObject>

DS_BEGIN_NAMESPACE

class DApplet;
class AppletConfigManager;
class DAppletLoaderPrivate;
class DAppletLoader : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    D_DECLARE_PRIVATE(DAppletLoader)
public:
    explicit DAppletLoader(DApplet *applet, AppletConfigManager *configManager, QObject *parent = nullptr);
    virtual ~DAppletLoader() override;

    void setPluginId(const QString &pluginId);
    void exec();
    void remove(const QString &pluginId);
    DApplet *applet() const;

Q_SIGNALS:
    void failed(const QString &pluginId);
};

DS_END_NAMESPACE
