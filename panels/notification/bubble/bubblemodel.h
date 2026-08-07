// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "notifyentity.h"
#include "expiretimer.h"

#include <QAbstractListModel>
#include <QHash>
#include <QQueue>

class QTimer;

namespace notification {

class BubbleItem;
class BubbleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum {
        AppName = Qt::UserRole + 1,
        Id,
        Body,
        Summary,
        IconName,
        CTime,
        TimeTip,
        BodyImagePath,
        DefaultAction,
        Actions,
        Urgency,
        ContentRowCount,
        BubbleCount
    } BubbleRole;

    explicit BubbleModel(QObject *parent = nullptr);
    ~BubbleModel() override;

Q_SIGNALS:
    // Emitted when a bubble reaches its expire timeout and should be closed.
    void bubbleExpired(qint64 id, uint bubbleId);

public:
    void push(BubbleItem *bubble);

    BubbleItem *replaceBubble(BubbleItem *bubble);
    bool isReplaceBubble(const BubbleItem *bubble) const;

    QList<BubbleItem *> items() const;

    Q_INVOKABLE void remove(int index);
    void remove(const BubbleItem *bubble);
    BubbleItem *removeById(qint64 id);
    void clear();

    // Pause/resume the expire timer of the hovered bubble so hovering
    // keeps the bubble on screen (0 clears the blocked bubble).
    void setBlockedId(qint64 id);

    BubbleItem *bubbleItem(int bubbleIndex) const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int displayRowCount() const;

    void clearInvalidBubbles();

private:
    void insertBubble(BubbleItem *bubble);
    void updateBubbleCount(int count);
    int replaceBubbleIndex(const BubbleItem *bubble) const;
    void updateBubbleTimeTip();
    void updateContentRowCount(int rowCount);

    int timeoutInterval(const BubbleItem *bubble) const;
    void startTimeout(BubbleItem *bubble);
    void stopTimeout(qint64 id);
    void pauseTimeout(qint64 id);
    void resumeTimeout(qint64 id);
    void stopAllTimeouts();
    void onTimeout(qint64 id);

private:
    QTimer *m_updateTimeTipTimer = nullptr;
    QTimer *m_processPendingTimer = nullptr;
    ExpireTimer m_expireTimer;
    QList<BubbleItem *> m_bubbles;
    QQueue<BubbleItem *> m_pendingBubbles;
    QHash<qint64, uint> m_timeoutBubbleIds;
    QHash<qint64, int> m_pausedRemaining;
    qint64 m_blockedId = NotifyEntity::InvalidId;
    int m_maxKeep{5};
    int m_contentRowCount{6};
};

}
