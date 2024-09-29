// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSharedData>

namespace notification {

class NotifyData;
class NotifyEntity
{
public:
    NotifyEntity();
    explicit NotifyEntity(qint64 id, const QString &appName);
    explicit NotifyEntity(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                                          const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout);
    NotifyEntity(const NotifyEntity &other);
    NotifyEntity(NotifyEntity &&other) noexcept;
    ~NotifyEntity();

    NotifyEntity &operator=(const NotifyEntity &other);
    NotifyEntity &operator=(NotifyEntity &&other);

    bool operator==(const NotifyEntity &other) const;

    bool isValid() const;

    qint64 id() const;
    void setId(qint64 id);

    QString appName() const;
    void setAppName(const QString &appName);

    QString body() const;
    void setBody(const QString &body);

    QString summary() const;
    void setSummary(const QString &summary);

    QString appIcon() const;
    void setAppIcon(const QString &appIcon);

    QStringList actions() const;
    QString actionsString() const;
    void setActionString(const QString &actionString);

    QVariantMap hints() const;
    QString hintsString() const;
    void setHintString(const QString &hintString);

    uint replacesId() const;
    void setReplacesId(uint replacesId);

    int expiredTimeout() const;
    void setExpiredTimeout(int mSecs);

    qint64 cTime() const;
    void setCTime(qint64 cTime);

    bool processed() const;
    int processedType() const;
    void setProcessedType(int processedType);

    uint bubbleId() const;
    void setBubbleId(qint64 bubbleId);

    bool enablePreview() const;
    void setEnablePreview(bool enable);

    QVariantMap toVariantMap() const;
    static NotifyEntity fromVariantMap(const QVariantMap &mapInfo);

    static int processedValue();
    static int removedValue();

private:
    static QString convertHintsToString(const QVariantMap &map);
    static QString convertActionsToString(const QStringList &actions);
    static QStringList parseAction(const QString &actions);
    static QVariantMap parseHint(const QString &hints);

private:
    QExplicitlySharedDataPointer<NotifyData> d;
};

}

