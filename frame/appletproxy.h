// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <DObject>

DS_BEGIN_NAMESPACE
/**
 * @brief Expose own interfaces for other applets.
 */
class DAppletProxyPrivate;
class DS_SHARE DAppletProxy : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    D_DECLARE_PRIVATE(DAppletProxy)
public:
    ~DAppletProxy() override;

protected:
    explicit DAppletProxy(QObject *parent = nullptr);
    explicit DAppletProxy(DAppletProxyPrivate &dd, QObject *parent = nullptr);
};

class DApplet;
class DAppletMetaProxyPrivate;
class DAppletMetaProxy : public DAppletProxy
{
    D_DECLARE_PRIVATE(DAppletMetaProxy)
public:
    explicit DAppletMetaProxy(QObject *meta, QObject *parent);

    virtual const QMetaObject *metaObject() const override;
    virtual void *qt_metacast(const char *clname) override;
    virtual int qt_metacall(QMetaObject::Call c, int id, void **argv) override;
};

DS_END_NAMESPACE
