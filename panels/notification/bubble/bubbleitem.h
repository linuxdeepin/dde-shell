// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "notifyentity.h"

#include <QObject>

namespace notification {

class NotifyEntity;
class BubbleItem : public QObject
{
    Q_OBJECT
public:
    enum Urgency {
        Low = 0, // 0-low
        Normal,  // 1-normal
        Critical // 2-critical (critical notification does not timeout)
    };

    enum ClosedReason {
        Expired = 1,
        Dismissed = 2,
        Closed = 3,
        Unknown = 4,
    };

    explicit BubbleItem(QObject *parent = nullptr);
    explicit BubbleItem(const NotifyEntity &entity, QObject *parent = nullptr);

public:
    void setEntity(const NotifyEntity &entity);

public:
    qint64 id() const;
    uint bubbleId() const;
    QString appName() const;
    QString appIcon() const;
    QString summary() const;
    QString body() const;
    uint replacesId() const;
    int urgency() const;
    QString bodyImagePath() const;
    qint64 ctime() const;

    bool hasDisplayAction() const;
    bool hasDefaultAction() const;
    QString defaultActionText() const;
    QString defaultActionId() const;
    QString firstActionText() const;
    QString firstActionId() const;
    QStringList actionTexts() const;
    QStringList actionIds() const;

    int level() const;
    void setLevel(int level);

    QString timeTip() const;
    void setTimeTip(const QString &timeTip);

signals:
    void levelChanged();
    void timeTipChanged();
    void expired(BubbleItem *);

private:
    int defaultActionIdIndex() const;
    int defaultActionTextIndex() const;
    QStringList displayActions() const;
    QString displayText() const;

private:
    NotifyEntity m_entity;

private:
    int m_level = 0;
    int m_urgency = Normal;
    QString m_timeTip;
    QString m_bodyImagePath;
    const int DefaultTimeOutMSecs = 5000;
};

}


