// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include <QQuickItem>

namespace notification {

class NotificationCenterPanel : public DS_NAMESPACE::DPanel
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged FINAL)
public:
    explicit NotificationCenterPanel(QObject *parent = nullptr);
    ~NotificationCenterPanel();

    virtual bool load() override;
    virtual bool init() override;

    bool visible() const;

Q_SIGNALS:
    void visibleChanged();

private:
    void setVisible(const bool visible);

private:
    bool m_visible = false;
};

}
