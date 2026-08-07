// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblemodel.h"

#include <notifysetting.h>

#include "bubbleitem.h"

#include <QTimer>
#include <QLoggingCategory>
#include <QImage>
#include <QTemporaryFile>
#include <QUrl>

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}

namespace notification {

static const int BlockItemTimeout = 1000;

BubbleModel::BubbleModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_updateTimeTipTimer(new QTimer(this))
    , m_processPendingTimer(new QTimer(this))
{
    m_updateTimeTipTimer->setInterval(1000);
    m_updateTimeTipTimer->setSingleShot(false);

    m_processPendingTimer->setInterval(300);
    m_processPendingTimer->setSingleShot(true);

    m_maxKeep = NotifySetting::instance()->bubbleCount() + 2; // max keep folds.

    connect(m_updateTimeTipTimer, &QTimer::timeout, this, &BubbleModel::updateBubbleTimeTip);
    connect(m_processPendingTimer, &QTimer::timeout, this, [this] {
        if (!m_pendingBubbles.isEmpty()) {
            auto bubble = m_pendingBubbles.dequeue();
            insertBubble(bubble);
            m_processPendingTimer->start();
        }
    });
    connect(&m_expireTimer, &ExpireTimer::expired, this, &BubbleModel::onTimeout);

    connect(NotifySetting::instance(), &NotifySetting::contentRowCountChanged, this, &BubbleModel::updateContentRowCount);
    connect(NotifySetting::instance(), &NotifySetting::bubbleCountChanged, this, &BubbleModel::updateBubbleCount);
}

BubbleModel::~BubbleModel()
{
    qDeleteAll(m_pendingBubbles);
    m_pendingBubbles.clear();
    qDeleteAll(m_bubbles);
    m_bubbles.clear();
}

void BubbleModel::push(BubbleItem *bubble)
{
    if (!m_updateTimeTipTimer->isActive()) {
        m_updateTimeTipTimer->start();
    }

    if (m_processPendingTimer->isActive()) {
        m_pendingBubbles.enqueue(bubble);
    } else {
        insertBubble(bubble);
        m_processPendingTimer->start();
    }
}

void BubbleModel::insertBubble(BubbleItem *bubble)
{
    // Retain only enough bubbles to show requested number + folded overlay space.
    // This offloads disappearance smoothly to the QML remove transition.
    if (m_bubbles.size() >= m_maxKeep) {
        beginRemoveRows(QModelIndex(), m_bubbles.size() - 1, m_bubbles.size() - 1);
        auto old = m_bubbles.takeLast();
        old->deleteLater();
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_bubbles.prepend(bubble);
    endInsertRows();

    startTimeout(bubble);
}

bool BubbleModel::isReplaceBubble(const BubbleItem *bubble) const
{
    return replaceBubbleIndex(bubble) >= 0;
}

BubbleItem *BubbleModel::replaceBubble(BubbleItem *bubble)
{
    Q_ASSERT(isReplaceBubble(bubble));
    const auto replaceIndex = replaceBubbleIndex(bubble);
    const auto oldBubble = m_bubbles[replaceIndex];

    stopTimeout(oldBubble->id());

    m_bubbles.replace(replaceIndex, bubble);
    Q_EMIT dataChanged(index(replaceIndex), index(replaceIndex));

    startTimeout(bubble);

    // If the replaced bubble was the hovered one, keep blocking the new one.
    if (m_blockedId == oldBubble->id())
        pauseTimeout(bubble->id());

    return oldBubble;
}

void BubbleModel::clear()
{
    if (m_processPendingTimer) {
        m_processPendingTimer->stop();
    }
    qDeleteAll(m_pendingBubbles);
    m_pendingBubbles.clear();

    stopAllTimeouts();

    if (m_bubbles.count() <= 0)
        return;
    beginResetModel();
    qDeleteAll(m_bubbles);
    m_bubbles.clear();
    endResetModel();

    m_updateTimeTipTimer->stop();
}

QList<BubbleItem *> BubbleModel::items() const
{
    return m_bubbles;
}

void BubbleModel::remove(int index)
{
    if (index < 0 || index >= m_bubbles.size())
        return;

    auto bubble = m_bubbles.takeAt(index);
    stopTimeout(bubble->id());

    beginRemoveRows(QModelIndex(), index, index);
    bubble->deleteLater();
    endRemoveRows();
}

void BubbleModel::remove(const BubbleItem *bubble)
{
    const auto index = m_bubbles.indexOf(bubble);
    if (index >= 0) {
        remove(index);
    }
}

BubbleItem *BubbleModel::removeById(qint64 id)
{
    for (const auto &item : m_bubbles) {
        if (item->id() == id) {
            remove(m_bubbles.indexOf(item));
            return item;
        }
    }

    return nullptr;
}

BubbleItem *BubbleModel::bubbleItem(int bubbleIndex) const
{
    if (bubbleIndex < 0 || bubbleIndex >= items().count())
        return nullptr;

    return items().at(bubbleIndex);
}

int BubbleModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return displayRowCount();
}

QVariant BubbleModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row >= m_bubbles.size() || !index.isValid())
        return {};

    switch (role) {
    case BubbleModel::AppName:
        return m_bubbles[row]->appName();
    case BubbleModel::Id:
        return m_bubbles[row]->id();
    case BubbleModel::Body:
        return m_bubbles[row]->body();
    case BubbleModel::Summary:
        return m_bubbles[row]->summary();
    case BubbleModel::IconName:
        return m_bubbles[row]->appIcon();
    case BubbleModel::CTime:
        return m_bubbles[row]->ctime();
    case BubbleModel::TimeTip:
        return m_bubbles[row]->timeTip();
    case BubbleModel::BodyImagePath:
        return m_bubbles[row]->bodyImagePath();
    case BubbleModel::DefaultAction:
        return m_bubbles[row]->defaultAction();
    case BubbleModel::Actions:
        return m_bubbles[row]->actions();
    case BubbleModel::Urgency:
        return m_bubbles[row]->urgency();
    case BubbleModel::ContentRowCount:
        return NotifySetting::instance()->contentRowCount();
    case BubbleModel::BubbleCount:
        return NotifySetting::instance()->bubbleCount();
    default:
        break;
    }
    return {};
}

QHash<int, QByteArray> BubbleModel::roleNames() const
{
    QHash<int, QByteArray> mapRoleNames;
    mapRoleNames[BubbleModel::AppName] = "appName";
    mapRoleNames[BubbleModel::Id] = "id";
    mapRoleNames[BubbleModel::Body] = "body";
    mapRoleNames[BubbleModel::Summary] = "summary";
    mapRoleNames[BubbleModel::IconName] = "iconName";
    mapRoleNames[BubbleModel::CTime] = "ctime";
    mapRoleNames[BubbleModel::TimeTip] = "timeTip";
    mapRoleNames[BubbleModel::Urgency] = "urgency";
    mapRoleNames[BubbleModel::BodyImagePath] = "bodyImagePath";
    mapRoleNames[BubbleModel::DefaultAction] = "defaultAction";
    mapRoleNames[BubbleModel::Actions] = "actions";
    mapRoleNames[BubbleModel::ContentRowCount] = "contentRowCount";
    mapRoleNames[BubbleModel::BubbleCount] = "bubbleCount";
    return mapRoleNames;
}

int BubbleModel::displayRowCount() const
{
    return m_bubbles.count();
}

void BubbleModel::updateBubbleCount(int count)
{
    m_maxKeep = count + 2;
    // We don't dynamically add/remove based on setting here anymore
    // to let QML handle Repeater logic fully mapped to the model size

    if (!m_bubbles.isEmpty()) {
        Q_EMIT dataChanged(index(0), index(m_bubbles.size() - 1), {BubbleModel::BubbleCount});
    }
}

void BubbleModel::clearInvalidBubbles()
{
    for (int i = m_bubbles.count() - 1; i >= 0; i--) {
        auto bubble = m_bubbles.at(i);
        if (!bubble->isValid()) {
            remove(bubble);
        }
    }
}

int BubbleModel::replaceBubbleIndex(const BubbleItem *bubble) const
{
    if (bubble->isReplace()) {
        for (int i = 0; i < m_bubbles.size(); i++) {
            auto item = m_bubbles[i];
            if (item->appName() != bubble->appName())
                continue;

            if (item->bubbleId() == bubble->bubbleId()) {
                return i;
            }
        }
    }
    return -1;
}

void BubbleModel::updateBubbleTimeTip()
{
    if (m_bubbles.isEmpty()) {
        m_updateTimeTipTimer->stop();
        return;
    }

    for (auto item : m_bubbles) {
        QString timeTip = NotifyEntity::formatRelativeTime(item->ctime());
        if (!timeTip.isEmpty()) {
            item->setTimeTip(timeTip);
        }
    }

    Q_EMIT dataChanged(index(0), index(m_bubbles.size() - 1), {BubbleModel::TimeTip});
}

void BubbleModel::updateContentRowCount(int rowCount)
{
    if (m_contentRowCount == rowCount)
        return;

    m_contentRowCount = rowCount;

    if (!m_bubbles.isEmpty()) {
        Q_EMIT dataChanged(index(0), index(m_bubbles.size() - 1), {BubbleModel::ContentRowCount});
    }
}

void BubbleModel::setBlockedId(qint64 id)
{
    if (id == m_blockedId)
        return;

    if (m_blockedId != NotifyEntity::InvalidId)
        resumeTimeout(m_blockedId);

    m_blockedId = id;

    if (id != NotifyEntity::InvalidId)
        pauseTimeout(id);
}

int BubbleModel::timeoutInterval(const BubbleItem *bubble) const
{
    return effectiveTimeout(bubble->urgency(), bubble->timeout());
}

void BubbleModel::startTimeout(BubbleItem *bubble)
{
    const auto id = bubble->id();
    stopTimeout(id);

    const int interval = timeoutInterval(bubble);
    if (interval <= 0)
        return;

    m_expireTimer.start(id, interval);
    m_timeoutBubbleIds.insert(id, bubble->bubbleId());
}

void BubbleModel::stopTimeout(qint64 id)
{
    m_expireTimer.stop(id);
    m_timeoutBubbleIds.remove(id);
    m_pausedRemaining.remove(id);
}

void BubbleModel::pauseTimeout(qint64 id)
{
    if (!m_expireTimer.contains(id))
        return;

    m_pausedRemaining.insert(id, m_expireTimer.remaining(id));
    m_expireTimer.stop(id);
}

void BubbleModel::resumeTimeout(qint64 id)
{
    auto it = m_pausedRemaining.find(id);
    if (it == m_pausedRemaining.end())
        return;

    auto remaining = it.value();
    m_pausedRemaining.erase(it);
    if (remaining < BlockItemTimeout)
        remaining = BlockItemTimeout;

    m_expireTimer.start(id, remaining);
}

void BubbleModel::stopAllTimeouts()
{
    m_blockedId = NotifyEntity::InvalidId;
    m_timeoutBubbleIds.clear();
    m_pausedRemaining.clear();
    m_expireTimer.stopAll();
}

void BubbleModel::onTimeout(qint64 id)
{
    const uint bubbleId = m_timeoutBubbleIds.take(id);
    Q_EMIT bubbleExpired(id, bubbleId);
}

} // notification
