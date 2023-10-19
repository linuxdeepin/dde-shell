// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "private/applet_p.h"

#include <QLoggingCategory>

DS_BEGIN_NAMESPACE

DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

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

void DApplet::init()
{
}

void DApplet::load()
{

}

DS_END_NAMESPACE
