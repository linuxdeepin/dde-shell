// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-output-manager-unstable-v2.h"
#include <QtWaylandClient/QWaylandClientExtension>

struct wl_output;

class TreelandOutputWatcher : public QWaylandClientExtensionTemplate<TreelandOutputWatcher>, public QtWayland::treeland_output_manager_v2
{
    Q_OBJECT
public:
    TreelandOutputWatcher(QObject *parent = nullptr);
    ~TreelandOutputWatcher();

protected:
    void treeland_output_manager_v2_primary_output(struct ::wl_output *output) override;
};
