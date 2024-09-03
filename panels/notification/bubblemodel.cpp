// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblemodel.h"
#include "notificationentity.h"

#include <QTimer>
#include <QLoggingCategory>
#include <QDateTime>
#include <QImage>
#include <QTemporaryFile>
#include <QUrl>

namespace notification {

Q_DECLARE_LOGGING_CATEGORY(notificationLog)

BubbleModel::BubbleModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_updateTimeTipTimer(new QTimer(this))
{
    m_updateTimeTipTimer->setInterval(1000);
    m_updateTimeTipTimer->setSingleShot(false);
    connect(m_updateTimeTipTimer, &QTimer::timeout, this, &BubbleModel::updateBubbleTimeTip);
}

BubbleModel::~BubbleModel()
{
    qDeleteAll(m_bubbles);
    m_bubbles.clear();
}

void BubbleModel::push(NotificationEntity *bubble)
{
    if (!m_updateTimeTipTimer->isActive()) {
        m_updateTimeTipTimer->start();
    }

    bool more = displayRowCount() >= BubbleMaxCount;
    if (more) {
        beginRemoveRows(QModelIndex(), BubbleMaxCount - 1, BubbleMaxCount - 1);
        endRemoveRows();
    }
    beginInsertRows(QModelIndex(), 0, 0);
    m_bubbles.prepend(bubble);
    endInsertRows();

    connect(bubble, &NotificationEntity::timeTipChanged, this, [this, bubble] {
        const auto row = m_bubbles.indexOf(bubble);
        if (row <= displayRowCount()) {
            Q_EMIT dataChanged(index(row), index(row), {BubbleModel::TimeTip});
        }
    });

    updateLevel();
}

bool BubbleModel::isReplaceBubble(const NotificationEntity *bubble) const
{
    return replaceBubbleIndex(bubble) >= 0;
}

NotificationEntity *BubbleModel::replaceBubble(NotificationEntity *bubble)
{
    Q_ASSERT(isReplaceBubble(bubble));
    const auto replaceIndex = replaceBubbleIndex(bubble);
    const auto oldBubble = m_bubbles[replaceIndex];
    m_bubbles.replace(replaceIndex, bubble);
    Q_EMIT dataChanged(index(replaceIndex), index(replaceIndex));

    return oldBubble;
}

void BubbleModel::clear()
{
    if (m_bubbles.count() <= 0)
        return;
    beginRemoveRows(QModelIndex(), 0, m_bubbles.count() - 1);
    m_bubbles.clear();
    endResetModel();

    updateLevel();
    m_updateTimeTipTimer->stop();
}

QList<NotificationEntity *> BubbleModel::items() const
{
    return m_bubbles;
}

void BubbleModel::remove(int index)
{
    if (index < 0 || index >= displayRowCount())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    auto bubble = m_bubbles.takeAt(index);
    bubble->deleteLater();
    endRemoveRows();

    if (m_bubbles.count() >= BubbleMaxCount) {
        beginInsertRows(QModelIndex(), displayRowCount() - 1, displayRowCount() - 1);
        endInsertRows();
        updateLevel();
    }
}

void BubbleModel::remove(const NotificationEntity *bubble)
{
    const auto index = m_bubbles.indexOf(bubble);
    if (index >= 0) {
        remove(index);
    }
}

NotificationEntity *BubbleModel::removeById(uint id)
{
    for (const auto &item : m_bubbles) {
        if (item->id() == id) {
            remove(m_bubbles.indexOf(item));
            return item;
        }
    }

    return nullptr;
}

NotificationEntity *BubbleModel::bubbleItem(int bubbleIndex) const
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
    case BubbleModel::Text:
        return m_bubbles[row]->text();
    case BubbleModel::Title:
        return m_bubbles[row]->title();
    case BubbleModel::IconName:
        return m_bubbles[row]->iconName();
    case BubbleModel::Level:
        return m_bubbles[row]->level();
    case BubbleModel::CTime:
        return m_bubbles[row]->ctime();
    case BubbleModel::TimeTip:
        return m_bubbles[row]->timeTip();
    case BubbleModel::OverlayCount:
        return overlayCount();
    case BubbleModel::hasDefaultAction:
        return m_bubbles[row]->hasDefaultAction();
    case BubbleModel::hasDisplayAction:
        return m_bubbles[row]->hasDisplayAction();
    case BubbleModel::FirstActionText:
        return m_bubbles[row]->firstActionText();
    case BubbleModel::FirstActionId:
        return m_bubbles[row]->firstActionId();
    case BubbleModel::ActionTexts:
        return m_bubbles[row]->actionTexts();
    case BubbleModel::ActionIds:
        return m_bubbles[row]->actionIds();
    case BubbleModel::Urgency:
        return m_bubbles[row]->urgency();
    default:
        break;
    }
    return {};
}

QHash<int, QByteArray> BubbleModel::roleNames() const
{
    QHash<int, QByteArray> mapRoleNames;
    mapRoleNames[BubbleModel::Text] = "text";
    mapRoleNames[BubbleModel::Title] = "title";
    mapRoleNames[BubbleModel::IconName] = "iconName";
    mapRoleNames[BubbleModel::Level] = "level";
    mapRoleNames[BubbleModel::CTime] = "ctime";
    mapRoleNames[BubbleModel::TimeTip] = "timeTip";
    mapRoleNames[BubbleModel::Urgency] = "urgency";
    mapRoleNames[BubbleModel::OverlayCount] = "overlayCount";
    mapRoleNames[BubbleModel::hasDefaultAction] = "hasDefaultAction";
    mapRoleNames[BubbleModel::hasDisplayAction] = "hasDisplayAction";
    mapRoleNames[BubbleModel::FirstActionText] = "firstActionText";
    mapRoleNames[BubbleModel::FirstActionId] = "firstActionId";
    mapRoleNames[BubbleModel::ActionTexts] = "actionTexts";
    mapRoleNames[BubbleModel::ActionIds] = "actionIds";
    return mapRoleNames;
}

int BubbleModel::displayRowCount() const
{
    return qMin(m_bubbles.count(), BubbleMaxCount);
}

int BubbleModel::overlayCount() const
{
    return qMin(m_bubbles.count() - displayRowCount(), OverlayMaxCount);
}

int BubbleModel::replaceBubbleIndex(const NotificationEntity *bubble) const
{
    if (bubble->replacesId() != NoReplaceId) {
        for (int i = 0; i < m_bubbles.size(); i++) {
            auto item = m_bubbles[i];
            if (item->appName() != item->appName())
                continue;

            const bool firstItem = item->replacesId() == NoReplaceId && item->id() == bubble->replacesId();
            const bool laterItem = item->replacesId() == bubble->replacesId();
            if (firstItem || laterItem) {
                return i;
            }
        }
    }
    return -1;
}

void BubbleModel::updateLevel()
{
    if (m_bubbles.isEmpty())
        return;

    for (int i = 0; i < displayRowCount(); i++) {
        auto item = m_bubbles.at(i);
        item->setLevel(i == LastBubbleMaxIndex ? 1 + overlayCount() : 1);
    }
    Q_EMIT dataChanged(index(0), index(displayRowCount() - 1), {BubbleModel::Level});
}

void BubbleModel::updateBubbleTimeTip()
{
    if (m_bubbles.isEmpty()) {
        m_updateTimeTipTimer->stop();
    }

    for (int i = 0; i < displayRowCount(); i++) {
        auto item = m_bubbles.at(i);

        qint64 diff = QDateTime::currentSecsSinceEpoch() - item->createdTimeSecs();
        QString timeTip;
        if (diff >= 60) {
            timeTip = tr("%1 minutes ago").arg(diff / 60);
            item->setTimeTip(timeTip);
        };
    }
}

}
