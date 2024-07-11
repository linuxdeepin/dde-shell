// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trayitempositionmanager.h"

#include <QTimer>
#include <QDebug>

namespace docktray {

void TrayItemPositionManager::registerVisualItemSize(int index, const QSize &size)
{
    while (m_registeredItemsSize.count() < (index + 1)) {
        m_registeredItemsSize.append(QSize(16, 16));
    }
    m_registeredItemsSize[index] = size;
}

QSize TrayItemPositionManager::visualItemSize(int index) const
{
    if (m_registeredItemsSize.count() <= index) return QSize(16, 16);
    return m_registeredItemsSize.at(index);
}

QSize TrayItemPositionManager::visualSize(int index, bool includeLastSpacing) const
{
    if (m_orientation == Qt::Horizontal) {
        int width = 0;
        for (int i = 0; i <= index; i++) {
            width += (visualItemSize(i).width() + 10);
        }
        return QSize((!includeLastSpacing && index > 0) ? (width - 10) : width, m_dockHeight);
    } else {
        int height = 0;
        for (int i = 0; i <= index; i++) {
            height += (visualItemSize(i).height() + 10);
        }
        return QSize(m_dockHeight, (!includeLastSpacing && index > 0) ? (height - 10) : height);
    }
}

DropIndex TrayItemPositionManager::itemIndexByPoint(const QPoint point) const
{
    if (m_orientation == Qt::Horizontal) {
        int pos = point.x();
        int width = 0;
        for (int i = 0; i < m_visualItemCount; i++) {
            int visualWidth = visualItemSize(i).width();
            if (pos < (width + visualWidth + 10)) {
                pos -= width;
                return DropIndex {
                    .index = i,
                    .isOnItem = pos <= visualWidth,
                    .isBefore = pos < (visualWidth / 2)
                };
            }
            width += (visualWidth + 10);
        }
        return DropIndex { .index = m_visualItemCount - 1 };
    } else {
        int pos = point.y();
        int height = 0;
        for (int i = 0; i <= m_visualItemCount; i++) {
            int visualHeight = visualItemSize(i).height();
            if (pos < (height + visualHeight + 10)) {
                pos -= height;
                return DropIndex {
                    .index = i,
                    .isOnItem = pos <= visualHeight,
                    .isBefore = pos < (visualHeight / 2)
                };
            }
            height += (visualHeight + 10);
        }
        return DropIndex { .index = m_visualItemCount - 1 };
    }
}

Qt::Orientation TrayItemPositionManager::orientation() const
{
    return m_orientation;
}

int TrayItemPositionManager::dockHeight() const
{
    return m_dockHeight;
}

// This should only be used to check layout issue or workaround layout issues.
// Do NOT rely on this to correct layout issue in a long run!
void TrayItemPositionManager::layoutHealthCheck(int delayMs)
{
    QTimer::singleShot(delayMs, [this](){
        if (m_dockHeight == 0) {
            qWarning() << "dock height is not valid, aborting layout health check...";
            return;
        }
        QSize result(visualSize(m_visualItemCount - 1, false));
        if (m_visualSize != result) {
            qWarning() << "layout size not matched, will trigger a force re-layout...";
            emit orientationChanged(m_orientation);
        } else {
            qDebug() << "no problem founded while performing layout health check!";
        }
    });
    qDebug() << "layout health check scheduled!";
}

TrayItemPositionManager::TrayItemPositionManager(QObject *parent)
    : QObject(parent)
{
    connect(this, &TrayItemPositionManager::visualItemCountChanged,
            this, &TrayItemPositionManager::updateVisualSize);
    connect(this, &TrayItemPositionManager::dockHeightChanged,
            this, &TrayItemPositionManager::updateVisualSize);
    connect(this, &TrayItemPositionManager::orientationChanged,
            this, &TrayItemPositionManager::updateVisualSize);
}

void TrayItemPositionManager::updateVisualSize()
{
    if (m_dockHeight == 0) return;
    QSize result(visualSize(m_visualItemCount - 1, false));
    qDebug() << "updateVisualSize()" << m_dockHeight << result;
    setProperty("visualSize", result);
}

}
