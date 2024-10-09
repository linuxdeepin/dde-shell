// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

namespace notification {

class NotificationManager;
class NotifyServerApplet : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
public:
    explicit NotifyServerApplet(QObject *parent = nullptr);

    bool load() override;
    bool init() override;

Q_SIGNALS:
    void notificationStateChanged(qint64 id, int processedType);

public Q_SLOTS:
    void actionInvoked(qint64 id, uint bubbleId, const QString &actionKey);
    void notificationClosed(qint64 id, uint bubbleId, uint reason);
    void notificationReplaced(qint64 id);

private:
    NotificationManager *m_manager = nullptr;
};

}
