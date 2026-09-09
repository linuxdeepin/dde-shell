// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandshowdesktop.h"
#include "wayland-treeland-show-desktop-unstable-v1-client-protocol.h"

namespace dock
{
TreelandShowDesktop::TreelandShowDesktop(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandShowDesktop>(treeland_show_desktop_v1_interface.version)
    , m_desktopState(state_normal)
{
    setParent(parent);
}

void TreelandShowDesktop::desktopToggle()
{
    if (isActive()) {
        set_show_desktop_state(m_desktopState == state_show ? state_normal : state_show);
    }
}

void TreelandShowDesktop::treeland_show_desktop_v1_show_desktop_state(uint32_t state)
{
    if (state != m_desktopState) {
        m_desktopState = state;
    }
}
}
