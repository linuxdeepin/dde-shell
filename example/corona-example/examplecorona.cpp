// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "examplecorona.h"

#include "pluginfactory.h"


ExampleCorona::ExampleCorona(QObject *parent)
    : DCorona(parent)
{
}

void ExampleCorona::load()
{
    DCorona::load();
}

void ExampleCorona::init()
{
    DCorona::init();
}

D_APPLET_CLASS(ExampleCorona)

#include "examplecorona.moc"
