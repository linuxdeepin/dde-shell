// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationpanel.h"

#include "notificationmanager.h"
#include "notificationentity.h"
#include "bubblemodel.h"
#include "pluginfactory.h"
#include "dbusadaptor.h"

#include <QLoggingCategory>

namespace notification {

Q_LOGGING_CATEGORY(notificationLog, "dde.shell.notification")

NotificationPanel::NotificationPanel(QObject *parent)
    : DPanel(parent)
    , m_bubbles(new BubbleModel(this))
{
}

NotificationPanel::~NotificationPanel()
{

}

bool NotificationPanel::load()
{
#ifndef QT_DEBUG
    return false;
#else
    return DPanel::load();
#endif
}

bool NotificationPanel::init()
{
    DPanel::init();

    m_manager = new NotificationManager(this);
    if (!m_manager->registerDbusService()) {
        qWarning(notificationLog) << QString("Can't register to the D-Bus object.");
        return false;
    }

    m_bubbles = m_manager->bubbleModel();

    auto adaptor = new DbusAdaptor(m_manager);
    auto ddeDbusAdaptor = new DDENotificationDbusAdaptor(m_manager);

    connect(m_bubbles, &BubbleModel::rowsInserted, this, &NotificationPanel::onBubbleCountChanged);
    connect(m_bubbles, &BubbleModel::rowsRemoved, this, &NotificationPanel::onBubbleCountChanged);

    return true;
}

bool NotificationPanel::visible() const
{
    return m_visible;
}

void NotificationPanel::setVisible(const bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;
    Q_EMIT visibleChanged();
}

BubbleModel *NotificationPanel::bubbles() const
{
    return m_bubbles;
}

void NotificationPanel::invokeDefaultAction(int bubbleIndex)
{
    m_manager->invokeDefaultAction(bubbleIndex);
}

void NotificationPanel::invokeAction(int bubbleIndex, const QString &actionId)
{
    m_manager->invokeAction(bubbleIndex, actionId);
}

void NotificationPanel::close(int bubbleIndex)
{
    m_manager->close(bubbleIndex);
}

void NotificationPanel::delayProcess(int bubbleIndex)
{
    m_manager->delayProcess(bubbleIndex);
}

void NotificationPanel::onBubbleCountChanged()
{
    bool isEmpty = m_bubbles->items().isEmpty();
    setVisible(!isEmpty);
}

D_APPLET_CLASS(NotificationPanel)

}

#include "notificationpanel.moc"
