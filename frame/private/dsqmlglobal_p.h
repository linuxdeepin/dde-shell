// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <DObject>

DS_BEGIN_NAMESPACE
class DApplet;

class DQmlGlobalPrivate;
class DQmlGlobal : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    D_DECLARE_PRIVATE(DQmlGlobal)
    Q_PROPERTY(DApplet* rootApplet READ rootApplet NOTIFY rootAppletChanged FINAL)
public:
    explicit DQmlGlobal(QObject *parent = nullptr);
    ~DQmlGlobal() override;

    Q_INVOKABLE DApplet *applet(const QString &pluginId) const;
    Q_INVOKABLE QList<DApplet *> appletList(const QString &pluginId) const;

    DApplet *rootApplet() const;

    static DQmlGlobal *instance();

signals:
    void rootAppletChanged();
};

DS_END_NAMESPACE
