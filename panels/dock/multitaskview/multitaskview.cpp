// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "multitaskview.h"
#include "pluginfactory.h"

#include <QBuffer>

#include <DDciIcon>
#include <DDBusSender>
#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

namespace dock {

MultiTaskView::MultiTaskView(QObject *parent)
    : DDockApplet(parent)
{
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, &MultiTaskView::onHasCompositeChanged);

    setVisible(true);
    setIcon("dcc-view");
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

void MultiTaskView::onHasCompositeChanged()
{
    // TODO
    // setVisible(DWindowManagerHelper::instance()->hasComposite());
}

QString MultiTaskView::displayName() const
{
    return tr("Multitasking View");
}

QString MultiTaskView::itemKey() const
{
    return QString("multitasking-view");
}

QString MultiTaskView::settingKey() const
{
    return QString("multitasking-view");
}

D_APPLET_CLASS(MultiTaskView)
}

#include "multitaskview.moc"
