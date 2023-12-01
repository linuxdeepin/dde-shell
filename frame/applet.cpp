// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "private/applet_p.h"

#include <QLoggingCategory>
#include <QUuid>

DS_BEGIN_NAMESPACE

DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DAppletPrivate::DAppletPrivate(DApplet *qq)
    : DTK_CORE_NAMESPACE::DObjectPrivate(qq)
{
}

DApplet::DApplet(QObject *parent)
    : DApplet(*new DAppletPrivate(this), parent)
{
}

DApplet::DApplet(DAppletPrivate &dd, QObject *parent)
    : QObject(parent)
    , DObject(dd)
{

}

DApplet::~DApplet()
{

}

QString DApplet::id() const
{
    D_DC(DApplet);
    return d->m_id;
}

void DApplet::setId(const QString &id)
{
    D_D(DApplet);
    d->m_id = id;
}

void DApplet::setMetaData(const DPluginMetaData &metaData)
{
    D_D(DApplet);
    d->m_metaData = metaData;
}

QString DApplet::pluginId() const
{
    D_DC(DApplet);
    return d->m_metaData.pluginId();
}

DPluginMetaData DApplet::pluginMetaData() const
{
    D_DC(DApplet);
    return d->m_metaData;
}

QObject *DApplet::rootObject() const
{
    D_DC(DApplet);
    return d->m_rootObject;
}

DApplet *DApplet::parentApplet() const
{
    return qobject_cast<DApplet *>(parent());
}

bool DApplet::load(const DAppletData &)
{
    return true;
}

bool DApplet::init()
{
    return true;
}

DS_END_NAMESPACE
