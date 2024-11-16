// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QAbstractListModel>

class QTimer;

namespace notification {

class BubbleItem;
class BubbleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum {
        AppName = Qt::UserRole + 1,
        Body,
        Summary,
        IconName,
        Level,
        CTime,
        TimeTip,
        BodyImagePath,
        OverlayCount,
        DefaultAction,
        Actions,
        Urgency,
    } BubbleRole;

    explicit BubbleModel(QObject *parent = nullptr);
    ~BubbleModel();

public:
    void push(BubbleItem *bubble);

    BubbleItem *replaceBubble(BubbleItem *bubble);
    bool isReplaceBubble(const BubbleItem *bubble) const;

    QList<BubbleItem *> items() const;

    Q_INVOKABLE void remove(int index);
    void remove(const BubbleItem *bubble);
    BubbleItem *removeById(qint64 id);
    void clear();

    BubbleItem *bubbleItem(int bubbleIndex) const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int displayRowCount() const;
    int overlayCount() const;
    void setBubbleCount(int count);

private:
    int replaceBubbleIndex(const BubbleItem *bubble) const;
    void updateLevel();
    void updateBubbleTimeTip();

private:
    QTimer *m_updateTimeTipTimer = nullptr;
    QList<BubbleItem *> m_bubbles;
    int BubbleMaxCount{3};
    const int LastBubbleMaxIndex{BubbleMaxCount - 1};
    const int OverlayMaxCount{2};
    const int NoReplaceId{0};
};

}
