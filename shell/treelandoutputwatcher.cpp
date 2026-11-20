// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandoutputwatcher.h"
#include "wayland-treeland-output-manager-unstable-v2-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>
#include <QHash>
#include <QLoggingCategory>

#include <qpa/qwindowsysteminterface.h>

Q_LOGGING_CATEGORY(ddeShellOutputWatcher, "dde-shell.output-watcher")

TreelandOutputWatcher::TreelandOutputWatcher(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandOutputWatcher>(treeland_output_manager_v2_interface.version)
{
    setParent(parent);

    connect(qApp, &QGuiApplication::screenAdded, this, &TreelandOutputWatcher::onScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &TreelandOutputWatcher::onScreenRemoved);

    m_uuidManager = new TreelandOutputUuidManager(this);
    connect(m_uuidManager, &TreelandOutputUuidManager::activeChanged, this, &TreelandOutputWatcher::onUuidManagerActiveChanged);
    onUuidManagerActiveChanged();
}

TreelandOutputWatcher::~TreelandOutputWatcher()
{
    if (isInitialized())
        destroy();

    for (const auto &obj : m_uuidRequests) {
        if (obj)
            delete obj.data();
    }
    m_uuidRequests.clear();
    m_uuidToScreen.clear();
}

void TreelandOutputWatcher::onUuidManagerActiveChanged()
{
    if (m_uuidManager && m_uuidManager->isActive()) {
        for (auto *screen : qApp->screens())
            requestUuid(screen);
    }
}

void TreelandOutputWatcher::requestUuid(QScreen *screen)
{
    if (!screen || !m_uuidManager || !m_uuidManager->isActive())
        return;

    if (m_uuidRequests.contains(screen))
        return;

    auto *waylandScreen = screen->nativeInterface<QNativeInterface::QWaylandScreen>();
    if (!waylandScreen || !waylandScreen->output())
        return;

    auto *raw = m_uuidManager->get_uuid(waylandScreen->output());
    if (!raw)
        return;

    auto *uuidObj = new TreelandOutputUuid(raw, this);
    m_uuidRequests.insert(screen, uuidObj);

    connect(uuidObj, &TreelandOutputUuid::uuidReceived, this, [this, screen](const QString &uuid) {
        if (auto obj = m_uuidRequests.take(screen))
            obj->deleteLater();
        if (!uuid.isEmpty())
            m_uuidToScreen.insert(uuid, screen);

        if (!m_pendingPrimaryUuid.isEmpty())
            applyPrimary(m_pendingPrimaryUuid);
    });
}

void TreelandOutputWatcher::onScreenAdded(QScreen *screen)
{
    requestUuid(screen);

    if (!m_pendingPrimaryUuid.isEmpty())
        applyPrimary(m_pendingPrimaryUuid);
}

void TreelandOutputWatcher::onScreenRemoved(QScreen *screen)
{
    for (auto it = m_uuidToScreen.begin(); it != m_uuidToScreen.end();) {
        if (it.value() == screen)
            it = m_uuidToScreen.erase(it);
        else
            ++it;
    }
    if (auto obj = m_uuidRequests.take(screen))
        delete obj.data();
}

void TreelandOutputWatcher::treeland_output_manager_v2_primary_output(const QString &uuid)
{
    applyPrimary(uuid);
}

void TreelandOutputWatcher::treeland_output_manager_v2_primary_output_failed(uint32_t reason)
{
    qCWarning(ddeShellOutputWatcher) << "primary_output_failed, reason:" << reason;
    m_pendingPrimaryUuid.clear();
}

void TreelandOutputWatcher::applyPrimary(const QString &uuid)
{
    if (uuid.isEmpty()) {
        m_pendingPrimaryUuid.clear();
        return;
    }

    auto it = m_uuidToScreen.find(uuid);
    if (it == m_uuidToScreen.end()) {
        m_pendingPrimaryUuid = uuid;
        return;
    }

    QPointer<QScreen> screen = it.value();
    if (!screen) {
        m_pendingPrimaryUuid = uuid;
        return;
    }
    if (qApp->primaryScreen() == screen) {
        m_pendingPrimaryUuid.clear();
        return;
    }

    m_pendingPrimaryUuid.clear();
    QWindowSystemInterface::handlePrimaryScreenChanged(screen->handle());
}
