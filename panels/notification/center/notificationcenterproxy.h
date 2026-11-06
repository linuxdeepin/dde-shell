// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace notification {
class NotificationCenterPanel;
class NotificationCenterProxy : public QObject
{
    Q_OBJECT
public:
    explicit NotificationCenterProxy(QObject *parent = nullptr);
    ~NotificationCenterProxy();

public Q_SLOTS:
    void Toggle();
    void Show();
    void Hide();

Q_SIGNALS:
    void VisibleChanged(bool visible);

private:
    NotificationCenterPanel *panel() const;
};
}
