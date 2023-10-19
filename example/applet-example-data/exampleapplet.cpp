// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exampleapplet.h"

#include "pluginfactory.h"

ExampleApplet::ExampleApplet(QObject *parent)
    : DApplet(parent)
    , m_mainText("Custom Applet")
{

}

QString ExampleApplet::mainText() const
{
    return m_mainText;
}

D_APPLET_CLASS(ExampleApplet)

#include "exampleapplet.moc"
