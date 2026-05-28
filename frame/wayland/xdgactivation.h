// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <DObject>
#include <QObject>
#include <QWindow>

DS_BEGIN_NAMESPACE

class XdgActivationPrivate;
class DS_SHARE XdgActivation : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_OBJECT
    D_DECLARE_PRIVATE(XdgActivation)
public:
    explicit XdgActivation(QObject *parent = nullptr);
    ~XdgActivation() override;

    bool isActive() const;

    void requestToken(QWindow *window = nullptr, const QString &appId = {});

Q_SIGNALS:
    void tokenReady(const QString &token);
};

DS_END_NAMESPACE
