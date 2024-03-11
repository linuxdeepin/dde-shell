// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "workspaceitem.h"
#include "pluginfactory.h"

#include <DDBusSender>

DS_BEGIN_NAMESPACE
namespace dock {

static DDBusSender workspaceDbus()
{
    return DDBusSender().service("org.deepin.dde.workspace")
        .path("/org/deepin/dde/workspace")
        .interface("org.deepin.dde.workspace");
}

WorkspaceItem::WorkspaceItem(QObject *parent)
    : DApplet(parent)
{

}

void WorkspaceItem::toggleWorkspace()
{
    workspaceDbus().method("Toggle").call();
}

D_APPLET_CLASS(WorkspaceItem)
}

DS_END_NAMESPACE

#include "workspaceitem.moc"
