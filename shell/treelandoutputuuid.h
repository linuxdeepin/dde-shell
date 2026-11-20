// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-output-uuid-unstable-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QObject>
#include <QString>

class TreelandOutputUuidManager
    : public QWaylandClientExtensionTemplate<TreelandOutputUuidManager>
    , public QtWayland::treeland_output_uuid_manager_v1
{
    Q_OBJECT
public:
    TreelandOutputUuidManager(QObject *parent = nullptr);
    ~TreelandOutputUuidManager();
};

class TreelandOutputUuid
    : public QObject
    , public QtWayland::treeland_output_uuid_v1
{
    Q_OBJECT
public:
    TreelandOutputUuid(struct ::treeland_output_uuid_v1 *object, QObject *parent = nullptr);
    ~TreelandOutputUuid() override;

    QString uuid() const { return m_uuid; }

Q_SIGNALS:
    void uuidReceived(const QString &uuid);

protected:
    void treeland_output_uuid_v1_uuid(const QString &uuid) override;

private:
    QString m_uuid;
};
