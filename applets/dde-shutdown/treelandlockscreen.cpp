// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandlockscreen.h"
#include "wayland-treeland-compositor-action-unstable-v1-client-protocol.h"

DS_BEGIN_NAMESPACE
namespace shutdown
{
TreeLandLockScreen::TreeLandLockScreen()
    : QWaylandClientExtensionTemplate<TreeLandLockScreen>(treeland_compositor_action_v1_interface.version)
{
}

TreeLandLockScreen::~TreeLandLockScreen()
{
    if (isInitialized()) {
        destroy();
    }
}

void TreeLandLockScreen::lock()
{
    if (isActive()) {
        trigger(action_lockscreen);
    }
}

void TreeLandLockScreen::shutdown()
{
    if (isActive()) {
        trigger(action_shutdown_menu);
    }
}

void TreeLandLockScreen::switchUser()
{
    if (isActive()) {
        trigger(action_show_user_switch);
    }
}
}
DS_END_NAMESPACE
