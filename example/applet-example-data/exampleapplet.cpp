// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exampleapplet.h"

#include "pluginfactory.h"
#include "appletitem.h"

ExampleApplet::ExampleApplet(QObject *parent)
    : DApplet(parent)
    , m_mainText("Custom Applet")
{

}

QString ExampleApplet::mainText() const
{
    return m_mainText;
}

void ExampleApplet::init()
{
    DApplet::init();

    DAppletItem *root = qobject_cast<DAppletItem *>(rootObject());
    Q_ASSERT(root);

    m_mainText = QString("%1 - w:%2;h:%3").arg(m_mainText).arg(root->width()).arg(root->height());
    Q_EMIT mainTextChanged();
}

D_APPLET_CLASS(ExampleApplet)

#include "exampleapplet.moc"
