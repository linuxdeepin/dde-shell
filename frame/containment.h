// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

#include <DObject>
#include <QVariant>

DS_BEGIN_NAMESPACE

/**
 * @brief 容器插件
 */
class DContainmentPrivate;
class Q_DECL_EXPORT DContainment : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject *> appletItems READ appletItems NOTIFY appletItemsChanged)
    D_DECLARE_PRIVATE(DContainment)
public:
    explicit DContainment(QObject *parent = nullptr);
    virtual ~DContainment() override;

    DApplet *createApplet(const QString &pluginId);

    QList<DApplet *> applets() const;
    QList<QObject *> appletItems();

    void load() override;
    void init() override;

protected:
    explicit DContainment(DContainmentPrivate &dd, QObject *parent = nullptr);

Q_SIGNALS:
    void appletItemsChanged();
};

DS_END_NAMESPACE
