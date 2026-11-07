// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandoutputuuid.h"
#include "wayland-treeland-output-uuid-unstable-v1-client-protocol.h"

TreelandOutputUuidManager::TreelandOutputUuidManager(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandOutputUuidManager>(treeland_output_uuid_manager_v1_interface.version)
{
    setParent(parent);
}

TreelandOutputUuidManager::~TreelandOutputUuidManager()
{
    if (isInitialized())
        destroy();
}

TreelandOutputUuid::TreelandOutputUuid(struct ::treeland_output_uuid_v1 *object, QObject *parent)
    : QObject(parent)
    , QtWayland::treeland_output_uuid_v1(object)
{
}

TreelandOutputUuid::~TreelandOutputUuid()
{
    if (isInitialized())
        destroy();
}

void TreelandOutputUuid::treeland_output_uuid_v1_uuid(const QString &uuid)
{
    m_uuid = uuid;
    Q_EMIT uuidReceived(uuid);
}
