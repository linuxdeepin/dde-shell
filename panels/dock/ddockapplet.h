// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"
#include "dsglobal.h"

#include <DObject>

#include <QVariant>

DS_BEGIN_NAMESPACE
/**
 * @brief You Can Used As Dock Plugins Sub Class
 */
class DDockAppletPrivate;
class DS_SHARE DDockApplet : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName CONSTANT FINAL)
    Q_PROPERTY(QString itemKey READ itemKey CONSTANT FINAL)
    Q_PROPERTY(QString settingKey READ settingKey CONSTANT FINAL)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL);

    D_DECLARE_PRIVATE(DDockApplet);

public:
    explicit DDockApplet(QObject *parent = nullptr);
    virtual ~DDockApplet() override;

    virtual QString name() const;
    virtual QString displayName() const = 0;
    virtual QString itemKey() const = 0;
    virtual QString settingKey() const = 0;

    QString icon() const;
    void setIcon(const QString &icon);

    bool visible() const;
    void setVisible(bool visible);

Q_SIGNALS:
    void iconChanged(const QString &);
    void visibleChanged(bool);

protected:
    explicit DDockApplet(DDockAppletPrivate &dd, QObject *parent = nullptr);
};

DS_END_NAMESPACE
