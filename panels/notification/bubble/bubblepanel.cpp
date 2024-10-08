// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblepanel.h"
#include "bubblemodel.h"
#include "pluginfactory.h"
#include "bubbleitem.h"

#include <QLoggingCategory>
#include <QQueue>

namespace notification {

Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notification")

BubblePanel::BubblePanel(QObject *parent)
    : DPanel(parent)
    , m_bubbles(new BubbleModel(this))
{
}

BubblePanel::~BubblePanel()
{

}

bool BubblePanel::load()
{
    return DPanel::load();
}

bool BubblePanel::init()
{
    DPanel::init();

    auto applets = appletList("org.deepin.ds.notificationserver");
    if (applets.isEmpty() || !applets.at(0)) {
        qCWarning(notificationLog) << "Can't get notification server object";
        return false;
    }

    m_notificationServer = applets.at(0);
    connect(m_notificationServer, SIGNAL(needShowEntity(const QVariantMap &)), this, SLOT(addBubble(const QVariantMap &)));
    connect(m_notificationServer, SIGNAL(needCloseEntity(uint)), this, SLOT(closeBubble(uint)));

    connect(m_bubbles, &BubbleModel::rowsInserted, this, &BubblePanel::onBubbleCountChanged);
    connect(m_bubbles, &BubbleModel::rowsRemoved, this, &BubblePanel::onBubbleCountChanged);

    return true;
}

bool BubblePanel::visible() const
{
    return m_visible;
}

BubbleModel *BubblePanel::bubbles() const
{
    return m_bubbles;
}

void BubblePanel::invokeDefaultAction(int bubbleIndex)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onActionInvoked(bubble->id(), bubble->bubbleId(), bubble->defaultActionId());
}

void BubblePanel::invokeAction(int bubbleIndex, const QString &actionId)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onActionInvoked(bubble->id(), bubble->bubbleId(), actionId);
}

void BubblePanel::close(int bubbleIndex)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onBubbleClosed(bubble->id(), bubble->bubbleId(), BubbleItem::Closed);
}

void BubblePanel::delayProcess(int bubbleIndex)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onBubbleClosed(bubble->id(), bubble->bubbleId(), BubbleItem::Dismissed);
}

void BubblePanel::onBubbleCountChanged()
{
    bool isEmpty = m_bubbles->items().isEmpty();
    setVisible(!isEmpty);
}

QList<DS_NAMESPACE::DApplet *> BubblePanel::appletList(const QString &pluginId) const
{
    QList<DS_NAMESPACE::DApplet *> ret;
    auto root = qobject_cast<DS_NAMESPACE::DContainment *>(parent());

    QQueue<DS_NAMESPACE::DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DS_NAMESPACE::DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DS_NAMESPACE::DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (applet->pluginId() == pluginId)
                ret << applet;
        }
    }
    return ret;
}

void BubblePanel::addBubble(const QVariantMap &entityInfo)
{
    auto entity = NotifyEntity::fromVariantMap(entityInfo);
    auto bubble = new BubbleItem(entity);
    connect(bubble, &BubbleItem::expired, this, &BubblePanel::onBubbleExpired);

    if (m_bubbles->isReplaceBubble(bubble)) {
        auto oldBubble = m_bubbles->replaceBubble(bubble);
        if (oldBubble) {
            QMetaObject::invokeMethod(m_notificationServer, "notificationReplaced", Qt::DirectConnection,
                                      Q_ARG(qint64, oldBubble->id()));
            oldBubble->deleteLater();
        }
    } else {
        m_bubbles->push(bubble);
    }
}

void BubblePanel::closeBubble(uint bubbleId)
{
    m_bubbles->removeByBubbleId(bubbleId);
}

void BubblePanel::onBubbleExpired(BubbleItem *bubble)
{
    if (!bubble) {
        return;
    }

    m_bubbles->remove(bubble);
    onBubbleClosed(bubble->id(), bubble->bubbleId(), BubbleItem::Expired);
}

void BubblePanel::onActionInvoked(qint64 id, uint bubbleId, const QString &actionId)
{
    QMetaObject::invokeMethod(m_notificationServer, "actionInvoked", Qt::DirectConnection,
                              Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(QString, actionId));
}

void BubblePanel::onBubbleClosed(qint64 id, uint bubbleId, uint reason)
{
    QMetaObject::invokeMethod(m_notificationServer, "notificationClosed", Qt::DirectConnection,
                              Q_ARG(qint64, id), Q_ARG(uint, bubbleId), Q_ARG(uint, reason));
}

void BubblePanel::setVisible(const bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;
    Q_EMIT visibleChanged();
}

BubbleItem *BubblePanel::bubbleItem(int index)
{
    if (index < 0 || index >= m_bubbles->items().count())
        return nullptr;
    return m_bubbles->items().at(index);
}

D_APPLET_CLASS(BubblePanel)

}

#include "bubblepanel.moc"
