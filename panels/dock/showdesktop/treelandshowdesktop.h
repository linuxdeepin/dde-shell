// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-show-desktop-unstable-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <cstdint>

namespace dock
{

class TreelandShowDesktop : public QWaylandClientExtensionTemplate<TreelandShowDesktop>, public QtWayland::treeland_show_desktop_v1
{
public:
    explicit TreelandShowDesktop(QObject *parent);

    void desktopToggle();

protected:
    void treeland_show_desktop_v1_show_desktop_state(uint32_t state) override;

private:
    uint32_t m_desktopState;
};
}
