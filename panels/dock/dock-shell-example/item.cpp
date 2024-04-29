// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "item.h"
#include "pluginfactory.h"

#include <DDBusSender>
#include <DDciIcon>
#include <DGuiApplicationHelper>

#include <QGuiApplication>
#include <QBuffer>

DGUI_USE_NAMESPACE

namespace dock {

Item::Item(QObject *parent)
    : DDockApplet(parent)
{
    // for debug
    setVisible(true);
}

void Item::activate()
{
    qDebug() << "item clicked";
}

QString Item::displayName() const
{
    return "dock-shell-example";
}

QString Item::itemKey() const
{
    return "dock-shell-example";
}

QString Item::settingKey() const
{
    return "dock-shell-example";
}

D_APPLET_CLASS(Item)
}


#include "item.moc"
