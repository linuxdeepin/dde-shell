// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include <QQuickItem>

namespace notification {

class BubbleItem;
class BubbleModel;
class NotificationEntity;
class NotificationManager;
class NotificationPanel : public DS_NAMESPACE::DPanel
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(BubbleModel *bubbles READ bubbles CONSTANT FINAL)
public:
    explicit NotificationPanel(QObject *parent = nullptr);
    ~NotificationPanel();

    virtual bool load() override;
    virtual bool init() override;

    bool visible() const;
    BubbleModel *bubbles() const;

public Q_SLOTS:
    void invokeDefaultAction(int bubbleIndex);
    void invokeAction(int bubbleIndex, const QString &actionId);
    void close(int bubbleIndex);
    void delayProcess(int bubbleIndex);

Q_SIGNALS:
    void visibleChanged();

private Q_SLOTS:
    void onBubbleCountChanged();

private:
    void setVisible(const bool visible);

private:
    bool m_visible = false;
    BubbleModel *m_bubbles = nullptr;
    NotificationManager *m_manager = nullptr;
};

}
