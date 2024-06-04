// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quickpanel.h"
#include "pluginfactory.h"

namespace dock {

QuickPanelApplet::QuickPanelApplet(QObject *parent)
    : DApplet(parent)
{

}

D_APPLET_CLASS(QuickPanelApplet)
}

#include "quickpanel.moc"
