// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyentity.h"

#include <QLoggingCategory>
#include <QVariant>

namespace notifycenter {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)

class NotifyEntity::Data : public QSharedData
{
public:
    Data()
        : QSharedData()
    {
    }
    virtual ~Data()
    {
    }
    QString id;
    QString appName;
    qint64 time = 0;
    QString iconName;
    QString title;
    QString content;
    QString action;
    QString hint;
};

NotifyEntity::NotifyEntity()
    : d(new NotifyEntity::Data())
{
}

NotifyEntity::NotifyEntity(const QString &id, const QString appName)
    : d(new NotifyEntity::Data())
{
    d->id = id;
    d->appName = appName;
}

NotifyEntity::NotifyEntity(const NotifyEntity &other)
    : d(other.d)
{
}

NotifyEntity &NotifyEntity::operator=(const NotifyEntity &other)
{
    if (this != &other)
        d = other.d;
    return *this;
}

NotifyEntity::NotifyEntity(NotifyEntity &&other)
{
    d = other.d;
    other.d = nullptr;
}

NotifyEntity &NotifyEntity::operator=(NotifyEntity &&other)
{
    d.swap(other.d);
    return *this;
}

NotifyEntity::~NotifyEntity()
{
}

bool NotifyEntity::operator==(const NotifyEntity &other) const
{
    if (d && other.d) {
        return id() == other.id();
    }
    return false;
}

bool NotifyEntity::isValid() const
{
    return d && !d->id.isEmpty();
}

QString NotifyEntity::appName() const
{
    return d->appName;
}

QString NotifyEntity::id() const
{
    return d->id;
}

QString NotifyEntity::content() const
{
    return d->content;
}

void NotifyEntity::setContent(const QString &content)
{
    d->content = content;
}

void NotifyEntity::setTime(qint64 time)
{
    d->time = time;
}

qint64 NotifyEntity::time() const
{
    return d->time;
}

void NotifyEntity::setAction(const QString &action)
{
    d->action = action;
}

QString NotifyEntity::action() const
{
    return d->action;
}

QString NotifyEntity::iconName() const
{
    return d->iconName;
}

void NotifyEntity::setIconName(const QString &newIconName)
{
    d->iconName = newIconName;
}

QString NotifyEntity::hint() const
{
    return d->hint;
}

void NotifyEntity::setHint(const QString &newHint)
{
    d->hint = newHint;
}

QString NotifyEntity::title() const
{
    return d->title;
}

void NotifyEntity::setTitle(const QString &newTitle)
{
    d->title = newTitle;
}

QVariantMap NotifyEntity::parseHint(const QString &hint)
{
    if (hint.isEmpty())
        return {};

    QVariantMap map;
    QStringList keyValueList = hint.split("|");
    foreach (QString text, keyValueList) {
        QStringList list = text.split("!!!");
        if (list.size() != 2)
            continue;
        QString key = list[0];
        QVariant value = QVariant::fromValue(list[1]);

        map.insert(key, value);
    }
    return map;
}

QStringList NotifyEntity::parseAction(const QString &action)
{
    if (action.isEmpty())
        return {};

    QStringList actions = action.split("|");
    Q_ASSERT((actions.size() % 2) != 1);
    return actions;
}
}
