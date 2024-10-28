// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblepanel.h"
#include "bubblemodel.h"
#include "pluginfactory.h"
#include "bubbleitem.h"

#include "sessionmanager1interface.h"
#include <QLoggingCategory>
#include <QQueue>

#include <DConfig>

#include <appletbridge.h>

#include "dbaccessor.h"

namespace notification {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)
}

namespace notification {
static const QString SessionDBusService = "org.deepin.dde.SessionManager1";
static const QString SessionDaemonDBusPath = "/org/deepin/dde/SessionManager1";

BubblePanel::BubblePanel(QObject *parent)
    : DPanel(parent)
    , m_bubbles(new BubbleModel(this))
{
}

BubblePanel::~BubblePanel()
{
    if (m_setting) {
        m_setting->deleteLater();
    }
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

    m_accessor = DBAccessor::instance();

    connect(m_notificationServer, SIGNAL(notificationStateChanged(qint64, int)), this, SLOT(onNotificationStateChanged(qint64, int)));

    m_setting = Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification");
    QObject::connect(m_setting, &Dtk::Core::DConfig::valueChanged, this, [this](const QString &key) {
        if (key == "bubbleCount") {
            updateMaxBubbleCount();
        }
    });
    updateMaxBubbleCount();
    connect(m_bubbles, &BubbleModel::rowsInserted, this, &BubblePanel::onBubbleCountChanged);
    connect(m_bubbles, &BubbleModel::rowsRemoved, this, &BubblePanel::onBubbleCountChanged);
    connect(m_bubbles, &BubbleModel::modelReset, this, &BubblePanel::onBubbleCountChanged);

    auto sessionManager = new org::deepin::dde::SessionManager1(SessionDBusService, SessionDaemonDBusPath, QDBusConnection::sessionBus(), this);
    if (!sessionManager->isValid()) {
        qWarning(notifyLog) << "Failed to connect SessionManager1:" << sessionManager->lastError().message();
    } else {
        m_locked = sessionManager->locked();
        QObject::connect(sessionManager, &org::deepin::dde::SessionManager1::LockedChanged, this, &BubblePanel::onLockedChanged);
    }

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
    onBubbleClosed(bubble->id(), bubble->bubbleId(), NotifyEntity::Closed);
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
    } else if (processedType == NotifyEntity::Processed) {
        qDebug(notifyLog) << "Close bubble for the notification" << id;
        closeBubble(id);
    }
}

void BubblePanel::onBubbleCountChanged()
{
    bool isEmpty = m_bubbles->items().isEmpty();
    const bool visible = !isEmpty && enabled();
    setVisible(visible);
}

void BubblePanel::onLockedChanged(bool locked)
{
    m_locked = locked;
    if (m_locked) {
        m_bubbles->clear();
    }
}

void BubblePanel::addBubble(qint64 id)
{
    const auto entity = m_accessor->fetchEntity(id);
    auto bubble = new BubbleItem(entity);
    const auto enabled = enablePreview(entity.appId());
    bubble->setEnablePreview(enabled);
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

void BubblePanel::closeBubble(qint64 id)
{
    m_bubbles->removeById(id);
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

BubbleItem *BubblePanel::bubbleItem(int index)
{
    if (index < 0 || index >= m_bubbles->items().count())
        return nullptr;
    return m_bubbles->items().at(index);
}

void BubblePanel::updateMaxBubbleCount()
{
    const auto count = m_setting->value("bubbleCount", 3).toInt();
    if (count < 1) {
        qWarning(notifyLog) << "Invalid settings of bubbleCount:" << count << ", it should be greater than 0";
        return;
    }
    m_bubbles->setBubbleCount(count);
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
