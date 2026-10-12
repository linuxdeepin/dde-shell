// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "qwayland-treeland-compositor-action-unstable-v1.h"

#include <QtWaylandClient/QWaylandClientExtension>

DS_BEGIN_NAMESPACE
namespace shutdown
{
class TreeLandLockScreen : public QWaylandClientExtensionTemplate<TreeLandLockScreen>, public QtWayland::treeland_compositor_action_v1
{
    Q_OBJECT

public:
    explicit TreeLandLockScreen();
    ~TreeLandLockScreen();
    void lock();
    void shutdown();
    void switchUser();
};
}
DS_END_NAMESPACE
