// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "multitaskview.h"
#include "pluginfactory.h"

#include <DDBusSender>
#include <DWindowManagerHelper>

DGUI_USE_NAMESPACE

namespace dock {

MultiTaskView::MultiTaskView(QObject *parent)
    : DApplet(parent)
    , m_iconName("deepin-multitasking-view")
{
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, &MultiTaskView::compositeStateChanged);
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

bool MultiTaskView::hasComposite()
{
    return DWindowManagerHelper::instance()->hasComposite();
}

D_APPLET_CLASS(MultiTaskView)
}


#include "multitaskview.moc"
