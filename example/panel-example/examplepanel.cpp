// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "examplepanel.h"

#include "pluginfactory.h"

ExamplePanel::ExamplePanel(QObject *parent)
    : DPanel(parent)
{
}

bool ExamplePanel::load(const DAppletData &data)
{
    return DPanel::load(data);
}

bool ExamplePanel::init()
{
    DPanel::init();
    Q_ASSERT(rootObject());
    Q_ASSERT(window());
    return true;
}

D_APPLET_CLASS(ExamplePanel)

#include "examplepanel.moc"
