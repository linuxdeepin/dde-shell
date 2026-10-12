// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-compositor-action-unstable-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>

namespace dock
{

class TreelandCompositorAction : public QWaylandClientExtensionTemplate<TreelandCompositorAction>, public QtWayland::treeland_compositor_action_v1
{
public:
    explicit TreelandCompositorAction(QObject *parent = nullptr);
    ~TreelandCompositorAction();
    void toggle();
};
}
