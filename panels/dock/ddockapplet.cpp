// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddockapplet.h"
#include "private/ddockapplet_p.h"

#include <QLoggingCategory>

DS_BEGIN_NAMESPACE

DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DDockAppletPrivate::DDockAppletPrivate(DDockApplet *qq)
    : DAppletPrivate(qq)
{

}

DDockAppletPrivate::~DDockAppletPrivate()
{

}

DDockApplet::DDockApplet(QObject *parent)
    : DDockApplet(*new DDockAppletPrivate(this), parent)
{

}

DDockApplet::DDockApplet(DDockAppletPrivate &dd, QObject *parent)
    : DApplet(dd, parent)
{

}

DDockApplet::~DDockApplet()
{
    qDebug(dsLog) << "Destroyed DDockApplet:" << pluginId() << id();
}

/*!
    A unique name that identifies the plug-in. The default implementation is `pluginId`.
*/
QString DDockApplet::name() const
{
    return DApplet::pluginId();
}

/*!
    A name that displays a name that matches the current locale
*/
/**
 * @brief DDockApplet::displayName
 * @return
 */

/**
 * @brief DDockApplet::itemKey
 * @return
 */

/**
 * @brief DDockApplet::settingKey
 * @return
 */

/*!
    A dci icon for display
*/
QString DDockApplet::icon() const
{
    D_DC(DDockApplet);

    return d->icon;
}

void DDockApplet::setIcon(const QString &icon)
{
    D_D(DDockApplet);

    d->icon = icon;
    Q_EMIT iconChanged(icon);
}

/*!
    Whether the plug-in is displayed on the panel
*/
bool DDockApplet::visible() const
{
    D_DC(DDockApplet);

    return d->visible;
}

void DDockApplet::setVisible(bool visible)
{
    D_D(DDockApplet);

    if (d->visible == visible)
        return;

    d->visible = visible;
    Q_EMIT visibleChanged(visible);
}

DS_END_NAMESPACE
