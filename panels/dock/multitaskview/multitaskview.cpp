// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "multitaskview.h"
#include "pluginfactory.h"

#include <DDBusSender>

namespace dock {

MultiTaskView::MultiTaskView(QObject *parent)
    : DApplet(parent)
    , m_iconName("deepin-multitasking-view")
{

}

bool MultiTaskView::init()
{
    DApplet::init();
    return true;
}

void MultiTaskView::openWorkspace()
{
    DDBusSender()
        .service("com.deepin.wm")
        .path("/com/deepin/wm")
        .interface("com.deepin.wm")
        .method("ShowWorkspace")
        .call();
}

QString MultiTaskView::iconName() const
{
    return m_iconName;
}

void MultiTaskView::setIconName(const QString& iconName)
{
    if (iconName != m_iconName) {
        m_iconName = iconName;
        Q_EMIT iconNameChanged();
    }
}

D_APPLET_CLASS(MultiTaskView)
}


#include "multitaskview.moc"
