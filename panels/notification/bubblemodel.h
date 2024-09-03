// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QAbstractListModel>

class QTimer;

namespace notification {

class NotificationEntity;
class BubbleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum {
        Text = Qt::UserRole + 1,
        Title,
        IconName,
        Level,
        CTime,
        TimeTip,
        OverlayCount,
        hasDefaultAction,
        hasDisplayAction,
        FirstActionText,
        FirstActionId,
        DefaultActionId,
        ActionTexts,
        ActionIds,
        Urgency,
    } BubbleRule;

    explicit BubbleModel(QObject *parent = nullptr);
    ~BubbleModel();

    void push(NotificationEntity *bubble);
    NotificationEntity *replaceBubble(NotificationEntity *bubble);
    bool isReplaceBubble(const NotificationEntity *bubble) const;
    void clear();
    QList<NotificationEntity *> items() const;
    Q_INVOKABLE void remove(int index);
    void remove(const NotificationEntity *bubble);
    NotificationEntity *removeById(uint id);
    NotificationEntity *bubbleItem(int bubbleIndex) const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int displayRowCount() const;
    int overlayCount() const;

private:
    int replaceBubbleIndex(const NotificationEntity *bubble) const;
    void updateLevel();
    void updateBubbleTimeTip();

private:
    QTimer *m_updateTimeTipTimer = nullptr;
    QList<NotificationEntity *> m_bubbles;
    const int BubbleMaxCount{3};
    const int LastBubbleMaxIndex{BubbleMaxCount - 1};
    const int OverlayMaxCount{2};
    const int NoReplaceId{0};
};

}
