// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clipboarditem.h"
#include "pluginfactory.h"

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
    : DDockApplet(parent)
{
    setVisible(true);
    setIcon("clipboard");
}

QString ClipboardItem::displayName() const
{
    return tr("Clipboard");
}

QString ClipboardItem::itemKey() const
{
    return QString("clipboard");
}

QString ClipboardItem::settingKey() const
{
       return QString("clipboard");
}

void ClipboardItem::toggleClipboard()
{
    clipboardDbus().method("Toggle").call();
}

D_APPLET_CLASS(ClipboardItem)
}


#include "clipboarditem.moc"
