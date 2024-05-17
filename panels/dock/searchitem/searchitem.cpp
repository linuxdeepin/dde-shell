// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddockapplet.h"
#include "searchitem.h"
#include "pluginfactory.h"

#include <DDBusSender>

#include <QProcess>

namespace dock {

static DDBusSender searchDbus()
{
    return DDBusSender().service("com.deepin.dde.GrandSearch")
        .path("/com/deepin/dde/GrandSearch")
        .interface("com.deepin.dde.GrandSearch");
}

SearchItem::SearchItem(QObject *parent)
    : DDockApplet(parent)
{
    setVisible(true);
    setIcon("search");
}

QString SearchItem::displayName() const
{
    return tr("GrandSearch");
}

QString SearchItem::itemKey() const
{
    return QString("search");
}

QString SearchItem::settingKey() const
{
    return QString("search");
}

void SearchItem::toggleGrandSearch()
{
    searchDbus().method("SetVisible").arg(true).call();
}

void SearchItem::toggleGrandSearchConfig()
{
    QProcess::startDetached("dde-grand-search", QStringList() << "--setting");
}

D_APPLET_CLASS(SearchItem)
}

#include "searchitem.moc"
