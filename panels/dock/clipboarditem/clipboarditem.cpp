// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "clipboarditem.h"
#include "pluginfactory.h"

#include <DDBusSender>

DS_BEGIN_NAMESPACE
namespace dock {

static DDBusSender clipboardDbus()
{
    return DDBusSender().service("org.deepin.dde.Clipboard1")
        .path("/org/deepin/dde/Clipboard1")
        .interface("org.deepin.dde.Clipboard1");
}

ClipboardItem::ClipboardItem(QObject *parent)
    : DApplet(parent)
{

}

void ClipboardItem::toggleClipboard()
{
    clipboardDbus().method("Toggle").call();
}

D_APPLET_CLASS(ClipboardItem)
}

DS_END_NAMESPACE

#include "clipboarditem.moc"
