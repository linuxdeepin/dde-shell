// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubbleitem.h"

#include <QUrl>
#include <QTimer>
#include <QImage>
#include <QBuffer>
#include <QDBusArgument>
#include <QTemporaryFile>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <DIconTheme>

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}

namespace notification {

BubbleItem::BubbleItem(QObject *parent)
    : QObject(parent)
    , m_timeTip(tr("just now"))
{

}

BubbleItem::BubbleItem(const NotifyEntity &entity, QObject *parent)
    : QObject(parent)
    , m_timeTip(tr("just now"))
{
    setEntity(entity);
}

void BubbleItem::setEntity(const NotifyEntity &entity)
{
    m_entity = entity;
    updateActions();

    QVariantMap hints = entity.hints();
    if (hints.contains("urgency")) {
        m_urgency = hints.value("urgency").toInt();
    }
}

qint64 BubbleItem::id() const
{
    return m_entity.id();
}

uint BubbleItem::bubbleId() const
{
    return m_entity.bubbleId();
}

QString BubbleItem::appName() const
{
    return m_entity.appName();
}

QString BubbleItem::appIcon() const
{
    return m_entity.appIconResolved();
}

QString BubbleItem::summary() const
{
    return m_entity.summary();
}

QString BubbleItem::body() const
{
    return enablePreview() ? m_entity.body() : tr("1 new message");
}

uint BubbleItem::replacesId() const
{
    return m_entity.replacesId();
}

bool BubbleItem::isReplace() const
{
    return m_entity.isReplace();
}

int BubbleItem::urgency() const
{
    return m_urgency;
}

QString BubbleItem::bodyImagePath() const
{
    return m_entity.bodyIcon();
}

qint64 BubbleItem::ctime() const
{
    return m_entity.cTime();
}

QString BubbleItem::defaultAction() const
{
    return m_defaultAction;
}

QVariantList BubbleItem::actions() const
{
    return m_actions;
}

void BubbleItem::updateActions()
{
    QStringList actions = m_entity.actions();
    const auto index = actions.indexOf(QLatin1String("default"));
    if (index >= 0) {
        // default Action maybe have text.
        m_defaultAction = QLatin1String("default");
        if (actions.size() % 2 == 1) {
            actions.remove(index);
        } else {
            actions.remove(index, 2);
        }
    }

    Q_ASSERT(actions.size() % 2 != 1);
    if (actions.size() % 2 == 1) {
        qWarning(notifyLog) << "Actions must be an even number except for default, The notify appName:" << m_entity.appName()
                            << ", actions:" << m_entity.actions();
        return;
    }

    QVariantList array;
    for (int i = 0; i < actions.size(); i += 2) {
        const auto id = actions[i];
        const auto text = actions[i + 1];
        QVariantMap item;
        item["id"] = id;
        item["text"] = text;
        item["enabled"] = true;
        array.append(item);
    }

    m_actions = array;
}


QString BubbleItem::timeTip() const
{
    return m_timeTip;
}

void BubbleItem::setTimeTip(const QString &timeTip)
{
    if (!timeTip.isEmpty() && timeTip != m_timeTip) {
        m_timeTip = timeTip;
        Q_EMIT timeTipChanged();
    }
}

bool BubbleItem::enablePreview() const
{
    return m_enablePreview;
}

void BubbleItem::setEnablePreview(bool enable)
{
    m_enablePreview = enable;
}

bool BubbleItem::isValid() const
{
    return m_entity.isValid();
}

QString BubbleItem::displayText() const
{
    return m_enablePreview ? m_entity.body() : tr("1 new message");
}

}
