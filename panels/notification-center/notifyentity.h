// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSharedData>
#include <QString>

namespace notifycenter {
/**
 * @brief The NotifyEntity class
 */
class NotifyEntity
{
    class Data;
public:
    NotifyEntity();
    NotifyEntity(const QString &id, const QString appName);
    NotifyEntity(const NotifyEntity &other);
    NotifyEntity &operator=(const NotifyEntity &other);
    NotifyEntity &operator=(NotifyEntity &&other);
    NotifyEntity(NotifyEntity &&other);
    virtual ~NotifyEntity();

    bool operator==(const NotifyEntity &other) const;
    bool isValid() const;

    QString appName() const;
    QString id() const;

    QString content() const;
    void setContent(const QString &content);

    qint64 time() const;
    void setTime(qint64 time);

    QString action() const;
    void setAction(const QString &action);

    QString iconName() const;
    void setIconName(const QString &newIconName);

    QString hint() const;
    void setHint(const QString &newHint);

    QString title() const;
    void setTitle(const QString &newTitle);

    static QVariantMap parseHint(const QString &hint);
    static QStringList parseAction(const QString &action);

private:
    QExplicitlySharedDataPointer<Data> d;
};
}
