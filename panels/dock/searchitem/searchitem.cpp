// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "searchitem.h"
#include "pluginfactory.h"

#include <DDBusSender>

#include <QProcess>

DS_BEGIN_NAMESPACE
namespace dock {

static DDBusSender searchDbus()
{
    return DDBusSender().service("com.deepin.dde.GrandSearch")
        .path("/com/deepin/dde/GrandSearch")
        .interface("com.deepin.dde.GrandSearch");
}

SearchItem::SearchItem(QObject *parent)
    : DApplet(parent)
{

}

void SearchItem::toggleGrandSearch()
{
    searchDbus().method("SetVisible").arg(true).call();
}

void SearchItem::toggleGrandSearchConfig()
{
    QProcess::startDetached("/usr/bin/dde-grand-search", QStringList() << "--setting");
}

D_APPLET_CLASS(SearchItem)
}

DS_END_NAMESPACE

#include "searchitem.moc"
