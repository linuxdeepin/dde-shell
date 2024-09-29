// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panel.h"
#include <QQuickItem>

namespace notification {

class NotifyEntity;
class BubbleModel;
class BubbleItem;

class BubblePanel : public DS_NAMESPACE::DPanel
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(BubbleModel *bubbles READ bubbles CONSTANT FINAL)
public:
    explicit BubblePanel(QObject *parent = nullptr);
    ~BubblePanel();

    bool load() override;
    bool init() override;

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
    void addBubble(const QVariantMap &entityInfo);
    void closeBubble(uint bubbleId);
    void onBubbleCountChanged();

private:
    QList<DS_NAMESPACE::DApplet *> appletList(const QString &pluginId) const;

private:
    void onBubbleExpired(BubbleItem *);
    void onActionInvoked(qint64 id, uint bubbleId, const QString &actionId);
    void onBubbleClosed(qint64 id, uint bubbleId, uint reason);
    void setVisible(const bool visible);

    BubbleItem *bubbleItem(int index);

private:
    bool m_visible = false;
    BubbleModel *m_bubbles = nullptr;
    DS_NAMESPACE::DApplet *m_notificationServer = nullptr;
};

}
