// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"

#include <QObject>
#include <QVariantMap>

namespace notification {

class NotificationEntity : public QObject
{
    Q_OBJECT
public:
    enum Urgency {
        Low = 0,
        Normal,
        Critical
    };

    explicit NotificationEntity(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                                const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout,
                                QObject *parent = nullptr);

    QString body() const;
    QString text() const;
    QString title() const;
    QString originIconName() const;
    QString iconName() const;
    QString appName() const;
    QStringList actions() const;
    QVariantMap hints() const;
    QString ctime() const;
    QString timeTip() const;
    int level() const;
    int id() const;
    uint replacesId() const;
    int timeout() const;
    int urgency() const;
    uint storageId() const;

    void setId(int id);
    void setReplacesId(uint replacesId);
    void setStorageId(uint storageId);
    void setLevel(int newLevel);
    void setEnablePreview(bool enable);

    qint64 createdTimeSecs() const;
    void setTimeTip(const QString &timeTip);

    QVariantMap toMap() const;

    bool hasDisplayAction() const;
    bool hasDefaultAction() const;
    QString defaultActionText() const;
    QString defaultActionId() const;
    QString firstActionText() const;
    QString firstActionId() const;
    QStringList actionTexts() const;
    QStringList actionIds() const;

Q_SIGNALS:
    void levelChanged();
    void timeTipChanged();
    void notificationTimeout(NotificationEntity *);

private:
    int defaultActionIdIndex() const;
    int defaultActionTextIndex() const;
    QStringList displayActions() const;
    QString displayText() const;

private:
    QString m_text;
    QString m_title;
    QString m_iconName;
    QString m_appName;
    QString m_timeTip;
    int m_id = 0;
    uint m_storageId = 0;
    QStringList m_actions;
    QVariantMap m_hints;
    uint m_replacesId;
    int m_timeout = 0;
    // 0-low 1-normal 2-critical (critical notification does not timeout)
    int m_urgency = Normal; // default
    QString m_ctime;
    bool m_showPreview = false;

private:
    int m_level = 0;
    const int TimeOutInterval = 5000;
};

} // notification
