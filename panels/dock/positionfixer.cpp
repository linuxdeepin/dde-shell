// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "positionfixer.h"
#include <QQuickWindow>
#include <cmath>

namespace dock {

PositionFixer::PositionFixer(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &PositionFixer::forceFix);
}

QQuickItem *PositionFixer::item() const
{
    return m_item;
}

void PositionFixer::setItem(QQuickItem *newItem)
{
    if (m_item == newItem)
        return;
    m_item = newItem;
    if (m_item && !m_container) {
        setContainer(m_item->parentItem());
    }
    emit itemChanged();
}

QQuickItem *PositionFixer::container() const
{
    return m_container;
}

void PositionFixer::setContainer(QQuickItem *newContainer)
{
    if (m_container == newContainer)
        return;
    m_container = newContainer;
    emit containerChanged();
}

void PositionFixer::fix()
{
    m_timer->start();
}

void PositionFixer::forceFix()
{
    if (!m_item || !m_container || !m_container->window()) {
        return;
    }

    QQuickItem *contentItem = m_container->window()->contentItem();
    if (!contentItem) {
        return;
    }

    QPointF scenePos = m_container->mapToItem(contentItem, QPointF(0, 0));
    
    qreal dpr = m_container->window()->devicePixelRatio();
    qreal physicalX = std::round(scenePos.x() * dpr);
    qreal physicalY = std::round(scenePos.y() * dpr);

    QQuickItem *itemParent = m_item->parentItem() ? m_item->parentItem() : m_container;

    QPointF localPosX = itemParent->mapFromItem(contentItem, QPointF(physicalX / dpr, scenePos.y()));
    QPointF localPosY = itemParent->mapFromItem(contentItem, QPointF(scenePos.x(), physicalY / dpr));
    m_item->setX(localPosX.x());
    m_item->setY(localPosY.y());
}

}
