// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblepanel.h"
#include "bubbleitem.h"
#include "bubblemodel.h"
#include "dataaccessorproxy.h"
#include "pluginfactory.h"

#include <QLoggingCategory>
#include <QQueue>

#include <appletbridge.h>

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}

namespace notification
{

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
    DS_NAMESPACE::DAppletBridge bridge("org.deepin.ds.notificationserver");
    m_notificationServer = bridge.applet();
    if (!m_notificationServer) {
        qCWarning(notifyLog) << "Can't get notification server object";
        return false;
    }

    m_accessor = DataAccessorProxy::instance();

    connect(m_notificationServer, SIGNAL(notificationStateChanged(qint64, int)), this, SLOT(onNotificationStateChanged(qint64, int)));

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

void BubblePanel::invokeAction(int bubbleIndex, const QString &actionId)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onActionInvoked(bubble->id(), bubble->bubbleId(), actionId);
}

void BubblePanel::close(int bubbleIndex, int reason)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onBubbleClosed(bubble->id(), bubble->bubbleId(), reason);
}

void BubblePanel::delayProcess(int bubbleIndex)
{
    auto bubble = bubbleItem(bubbleIndex);
    if (!bubble)
        return;

    m_bubbles->remove(bubbleIndex);
    onBubbleClosed(bubble->id(), bubble->bubbleId(), NotifyEntity::Dismissed);
}

void BubblePanel::onNotificationStateChanged(qint64 id, int processedType)
{
    if (processedType == NotifyEntity::NotProcessed) {
        qDebug(notifyLog) << "Add bubble for the notification" << id;
        addBubble(id);
    } else if (processedType == NotifyEntity::Processed || processedType == NotifyEntity::Removed) {
        qDebug(notifyLog) << "Close bubble for the notification" << id;
        closeBubble(id);
    }
}

void BubblePanel::onBubbleCountChanged()
{
    bool isEmpty = m_bubbles->items().isEmpty();
    setVisible(!isEmpty && enabled());
}

void BubblePanel::addBubble(qint64 id)
{
    const auto entity = m_accessor->fetchEntity(id);

    // Validate entity before creating bubble to prevent invalid notification banners
    if (!entity.isValid()) {
        qWarning(notifyLog) << "Failed to add bubble: invalid entity for id" << id << "appName:" << entity.appName() << "cTime:" << entity.cTime();
        return;
    }

    auto bubble = new BubbleItem(entity);
    const auto enabled = enablePreview(entity.appId());
    bubble->setEnablePreview(enabled);
    if (m_bubbles->isReplaceBubble(bubble)) {
        auto oldBubble = m_bubbles->replaceBubble(bubble);
        if (oldBubble) {
            oldBubble->deleteLater();
        }
    } else {
        m_bubbles->push(bubble);
    }
}

void BubblePanel::closeBubble(qint64 id)
{
    const auto entity = m_accessor->fetchEntity(id);
    if (entity.isValid()) {
        id = entity.bubbleId();
    } else {
        qDebug(notifyLog) << "Entity not found or invalid for close bubble, using storage id lookup:" << id;
        id = m_bubbles->getBubbleIdByStorageId(id);
    }

    if (id > 0) {
        m_bubbles->removeById(id);
    } else {
        qWarning(notifyLog) << "Failed to close bubble: invalid bubble id for entity id:" << id << "appName:" << entity.appName() << "cTime:" << entity.cTime();
        clearInvalidBubbles();
    }
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

bool BubblePanel::enablePreview(const QString &appId) const
{
    static const int EnablePreview = 3;
    QVariant value;
    QMetaObject::invokeMethod(m_notificationServer, "appValue", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, value), Q_ARG(const QString &, appId), Q_ARG(int, EnablePreview));
    return value.toBool();
}

void BubblePanel::clearInvalidBubbles()
{
    m_bubbles->clearInvalidBubbles();
}

BubbleItem *BubblePanel::bubbleItem(int index)
{
    if (index < 0 || index >= m_bubbles->items().count())
        return nullptr;
    return m_bubbles->items().at(index);
}

D_APPLET_CLASS(BubblePanel)

bool BubblePanel::enabled() const
{
    return m_enabled;
}

void BubblePanel::setEnabled(bool newEnabled)
{
    if (m_enabled == newEnabled)
        return;
    m_enabled = newEnabled;
    emit enabledChanged();

    bool isEmpty = m_bubbles->items().isEmpty();
    setVisible(!isEmpty && enabled());
}

}

#include "bubblepanel.moc"
