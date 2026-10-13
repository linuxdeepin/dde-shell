// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandcompositoraction.h"
#include "wayland-treeland-compositor-action-unstable-v1-client-protocol.h"

namespace dock
{
TreelandCompositorAction::TreelandCompositorAction(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandCompositorAction>(treeland_compositor_action_v1_interface.version)
{
    setParent(parent);
}

TreelandCompositorAction::~TreelandCompositorAction()
{
    if (isInitialized()) {
        destroy();
    }
}

void TreelandCompositorAction::toggle()
{
    if (isActive()) {
        trigger(action_toggle_multitask_view);
    }
}
}
