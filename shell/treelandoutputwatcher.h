// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "treelandoutputuuid.h"
#include "qwayland-treeland-output-manager-unstable-v2.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QString>
#include <QHash>
#include <QPointer>

class QScreen;

class TreelandOutputWatcher : public QWaylandClientExtensionTemplate<TreelandOutputWatcher>, public QtWayland::treeland_output_manager_v2
{
    Q_OBJECT
public:
    TreelandOutputWatcher(QObject *parent = nullptr);
    ~TreelandOutputWatcher();

protected:
    void treeland_output_manager_v2_primary_output(const QString &uuid) override;
    void treeland_output_manager_v2_primary_output_failed(uint32_t reason) override;

private:
    void onScreenAdded(QScreen *screen);
    void onScreenRemoved(QScreen *screen);
    void applyPrimary(const QString &uuid);
    void requestUuid(QScreen *screen);
    void onUuidManagerActiveChanged();

    TreelandOutputUuidManager *m_uuidManager = nullptr;
    QHash<QScreen *, QPointer<TreelandOutputUuid>> m_uuidRequests;
    QHash<QString, QPointer<QScreen>> m_uuidToScreen;
    QString m_pendingPrimaryUuid;
};
