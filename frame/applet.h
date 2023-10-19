// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "pluginmetadata.h"

#include <DObject>
#include <QVariant>

DS_BEGIN_NAMESPACE
/**
 * @brief 插件项，单个插件实例
 */
class DAppletPrivate;
class Q_DECL_EXPORT DApplet : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    Q_PROPERTY(QString pluginId READ pluginId CONSTANT FINAL)
    D_DECLARE_PRIVATE(DApplet)
public:
    explicit DApplet(QObject *parent = nullptr);
    virtual ~DApplet() override;

    void setMetaData(const DPluginMetaData &metaData);
    QString pluginId() const;

    DPluginMetaData pluginMetaData() const;

    virtual void init();

    virtual void load();

protected:
    explicit DApplet(DAppletPrivate &dd, QObject *parent = nullptr);
};

DS_END_NAMESPACE
