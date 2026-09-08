// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandoutputwatcher.h"
#include "wayland-treeland-output-manager-v1-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>

#include <qpa/qwindowsysteminterface.h>

TreelandOutputWatcher::TreelandOutputWatcher(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandOutputWatcher>(treeland_output_manager_v1_interface.version)
{
    setParent(parent);

    connect(qApp, &QGuiApplication::screenAdded, this, &TreelandOutputWatcher::onScreenAdded);
}

TreelandOutputWatcher::~TreelandOutputWatcher()
{
    destroy();
}

void TreelandOutputWatcher::onScreenAdded(QScreen *)
{
    if (!m_pendingPrimaryName.isEmpty())
        applyPrimary(m_pendingPrimaryName);
}

void TreelandOutputWatcher::treeland_output_manager_v1_primary_output(const QString &output_name)
{
    applyPrimary(output_name);
}

void TreelandOutputWatcher::applyPrimary(const QString &output_name)
{
    if (qApp->primaryScreen() && qApp->primaryScreen()->name() == output_name) {
        m_pendingPrimaryName.clear();
        return;
    }

    for (auto screen : qApp->screens()) {
        if (screen->name() == output_name) {
            m_pendingPrimaryName.clear();
            QWindowSystemInterface::handlePrimaryScreenChanged(screen->handle());
            return;
        }
    }

    m_pendingPrimaryName = output_name;
}
