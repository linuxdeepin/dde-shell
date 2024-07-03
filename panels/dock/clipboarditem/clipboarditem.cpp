// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "clipboarditem.h"
#include "pluginfactory.h"
#include "../constants.h"

#include <DDBusSender>
#include <DDciIcon>
#include <DGuiApplicationHelper>

#include <QGuiApplication>
#include <QBuffer>

DGUI_USE_NAMESPACE

namespace dock {

static DDBusSender clipboardDbus()
{
    return DDBusSender().service("org.deepin.dde.Clipboard1")
        .path("/org/deepin/dde/Clipboard1")
        .interface("org.deepin.dde.Clipboard1");
}

ClipboardItem::ClipboardItem(QObject *parent)
    : DApplet(parent)
    , m_visible(true)
{

}

void ClipboardItem::toggleClipboard()
{
    clipboardDbus().method("Toggle").call();
}

DockItemInfo ClipboardItem::dockItemInfo()
{
    DockItemInfo info;
    info.name = "clipboard";
    info.displayName = tr("Clipboard");
    info.itemKey = "clipboard";
    info.settingKey = "clipboard";
    info.visible = m_visible;
    info.dccIcon = DCCIconPath + "clipboard.svg";
    return info;
}

void ClipboardItem::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;

        Q_EMIT visibleChanged(visible);
    }
}

D_APPLET_CLASS(ClipboardItem)
}


#include "clipboarditem.moc"
