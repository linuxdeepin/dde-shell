// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exampleapplet.h"

#include "pluginfactory.h"

#include <DIconButton>
#include <DIconTheme>
#include <dplatformwindowhandle.h>

#include <QVBoxLayout>
#include <QWidget>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

WidgetExampleApplet::WidgetExampleApplet(QObject *parent)
    : DApplet(parent)
{
}

bool WidgetExampleApplet::init()
{
    auto widget = new QWidget();
    DPlatformWindowHandle handle(widget);
    widget->setFixedSize(QSize(200, 200));
    auto layout = new QVBoxLayout(widget);
    auto btn = new DIconButton();
    btn->setIcon(DIconTheme::findQIcon("deepin-home"));
    btn->setIconSize(QSize(36, 36));
    layout->addWidget(btn);
    widget->show();

    return DApplet::init();
}

D_APPLET_CLASS(WidgetExampleApplet)

#include "exampleapplet.moc"
