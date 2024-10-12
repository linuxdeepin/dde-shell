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

    bool enablePreview() const;
    void setEnablePreview(bool enable);

signals:
    void levelChanged();
    void timeTipChanged();

private:
    int defaultActionIdIndex() const;
    int defaultActionTextIndex() const;
    QStringList displayActions() const;
    QString displayText() const;

private:
    NotifyEntity m_entity;

private:
    int m_level = 0;
    int m_urgency = NotifyEntity::Normal;
    QString m_timeTip;
    bool m_enablePreview = true;
};

}


