// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandoutputwatcher.h"
#include "wayland-treeland-output-manager-unstable-v2-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>

#include <qpa/qwindowsysteminterface.h>

TreelandOutputWatcher::TreelandOutputWatcher(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandOutputWatcher>(treeland_output_manager_v2_interface.version)
{
    setParent(parent);
}

TreelandOutputWatcher::~TreelandOutputWatcher()
{
    destroy();
}

void TreelandOutputWatcher::treeland_output_manager_v2_primary_output(struct ::wl_output *output)
{
    if (!output)
        return;

    for (auto screen : qApp->screens()) {
        auto *waylandScreen = screen->nativeInterface<QNativeInterface::QWaylandScreen>();
        if (waylandScreen && waylandScreen->output() == output) {
            QWindowSystemInterface::handlePrimaryScreenChanged(screen->handle());
            return;
        }
    }
}
