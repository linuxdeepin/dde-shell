// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waylanddockhelper.h"

DS_BEGIN_NAMESPACE
namespace dock {
WaylandDockHelper::WaylandDockHelper(DockPanel* panel)
    : DockHelper(panel)
{

}

void WaylandDockHelper::updateDockTriggerArea()
{
    
}

bool WaylandDockHelper::mouseInDockArea()
{
    return false;
}
}
DS_END_NAMESPACE
