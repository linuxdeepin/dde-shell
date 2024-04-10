// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "searchitem.h"
#include "pluginfactory.h"

#include <DDBusSender>
#include <DDciIcon>

#include <QProcess>
#include <QBuffer>
#include <QGuiApplication>

DGUI_USE_NAMESPACE
namespace dock {

static DDBusSender searchDbus()
{
    return DDBusSender().service("com.deepin.dde.GrandSearch")
        .path("/com/deepin/dde/GrandSearch")
        .interface("com.deepin.dde.GrandSearch");
}

SearchItem::SearchItem(QObject *parent)
    : DApplet(parent)
    , m_visible(true)
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

DockItemInfo SearchItem::dockItemInfo()
{
    DockItemInfo info;
    info.name = "search";
    info.displayName = tr("GrandSearch");
    info.itemKey = "search";
    info.settingKey = "search";
    info.visible = m_visible;
    {
        auto lightPixmap = DDciIcon::fromTheme("search").pixmap(qApp->devicePixelRatio(), 30, DDciIcon::Light);
        QBuffer buffer(&info.iconLight);
        if (buffer.open(QIODevice::WriteOnly)) {
            lightPixmap.save(&buffer, "png");
        }
    }
    {
        auto darkPixmap = DDciIcon::fromTheme("search").pixmap(qApp->devicePixelRatio(), 30, DDciIcon::Dark);
        QBuffer buffer(&info.iconDark);
        if (buffer.open(QIODevice::WriteOnly)) {
            darkPixmap.save(&buffer, "png");
        }
    }

    return info;
}

void SearchItem::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;

        Q_EMIT visibleChanged(visible);
    }
}

D_APPLET_CLASS(SearchItem)
}


#include "searchitem.moc"
