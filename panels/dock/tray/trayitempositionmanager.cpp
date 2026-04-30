// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trayitempositionmanager.h"

#include <QTimer>
#include <QDebug>

namespace docktray {

// style of UI.
static const int itemSize = 16;
static const int itemPadding = 4;
static const int itemSpacing = 2;
static const QSize itemVisualSize = QSize(itemSize + itemPadding * 2, itemSize + itemPadding * 2);

void TrayItemPositionManager::beginLayoutSync()
{
    m_layoutSyncActive = true;
    m_visualItemSizeChangedPending = false;
}

void TrayItemPositionManager::endLayoutSync()
{
    m_layoutSyncActive = false;
    if (m_visualItemSizeChangedPending) {
        m_visualItemSizeChangedPending = false;
        emit visualItemSizeChanged();
    }
}

void TrayItemPositionManager::registerSurfaceSize(const QString &surfaceId, const QSize &size)
{
    if (surfaceId.isEmpty() || size.isEmpty()) {
        return;
    }

    const QSize normalizedSize = size.isValid() ? size : itemVisualSize;
    bool changed = m_registeredSurfaceSizes.value(surfaceId) != normalizedSize;
    m_registeredSurfaceSizes.insert(surfaceId, normalizedSize);

    for (int index = 0; index < m_registeredItemSurfaceIds.count(); ++index) {
        if (m_registeredItemSurfaceIds.at(index) != surfaceId) {
            continue;
        }

        if (m_registeredItemsSize.at(index) == normalizedSize) {
            continue;
        }

        m_registeredItemsSize[index] = normalizedSize;
        changed = true;
    }

    if (changed) {
        notifyVisualItemSizeChanged();
    }
}

void TrayItemPositionManager::registerVisualItem(const QString &surfaceId, int index)
{
    if (surfaceId.isEmpty() || index < 0) {
        return;
    }

    ensureRegisteredItemCapacity(index + 1);

    bool changed = false;
    for (int currentIndex = 0; currentIndex < m_registeredItemSurfaceIds.count(); ++currentIndex) {
        if (currentIndex == index || m_registeredItemSurfaceIds.at(currentIndex) != surfaceId) {
            continue;
        }

        m_registeredItemSurfaceIds[currentIndex].clear();
        if (m_registeredItemsSize.at(currentIndex) != itemVisualSize) {
            m_registeredItemsSize[currentIndex] = itemVisualSize;
            changed = true;
        }
    }

    const QSize size = m_registeredSurfaceSizes.value(surfaceId, itemVisualSize);
    if (m_registeredItemSurfaceIds.at(index) != surfaceId || m_registeredItemsSize.at(index) != size) {
        m_registeredItemSurfaceIds[index] = surfaceId;
        m_registeredItemsSize[index] = size;
        changed = true;
    }

    if (changed) {
        notifyVisualItemSizeChanged();
    }
}

void TrayItemPositionManager::unregisterVisualItem(const QString &surfaceId)
{
    if (surfaceId.isEmpty()) {
        return;
    }

    bool changed = false;
    for (int index = 0; index < m_registeredItemSurfaceIds.count(); ++index) {
        if (m_registeredItemSurfaceIds.at(index) != surfaceId) {
            continue;
        }

        m_registeredItemSurfaceIds[index].clear();
        if (m_registeredItemsSize.at(index) != itemVisualSize) {
            m_registeredItemsSize[index] = itemVisualSize;
            changed = true;
        }
    }

    if (changed) {
        notifyVisualItemSizeChanged();
    }
}

void TrayItemPositionManager::registerVisualItemSize(int index, const QSize &size)
{
    ensureRegisteredItemCapacity(index + 1);
    QSize oldSize = m_registeredItemsSize[index];
    m_registeredItemsSize[index] = size;

    // The registered itemsize may change, and the layout needs to be updated when it does.
    if (oldSize != size) {
        notifyVisualItemSizeChanged();
    }
}

QSize TrayItemPositionManager::visualItemSize(int index) const
{
    if (m_registeredItemsSize.count() <= index) return itemVisualSize;
    return m_registeredItemsSize.at(index);
}

QSize TrayItemPositionManager::visualSize(int index, bool includeLastSpacing) const
{
    if (m_orientation == Qt::Horizontal) {
        int width = 0;
        for (int i = 0; i <= index; i++) {
            width += (visualItemSize(i).width() + itemSpacing);
        }
        return QSize((!includeLastSpacing && index > 0) ? (width - itemSpacing) : width, m_dockHeight);
    } else {
        int height = 0;
        for (int i = 0; i <= index; i++) {
            height += (visualItemSize(i).height() + itemSpacing);
        }
        return QSize(m_dockHeight, (!includeLastSpacing && index > 0) ? (height - itemSpacing) : height);
    }
}

DropIndex TrayItemPositionManager::itemIndexByPoint(const QPoint point) const
{
    if (m_orientation == Qt::Horizontal) {
        int pos = point.x();
        int width = 0;
        for (int i = 0; i < m_visualItemCount; i++) {
            int visualWidth = visualItemSize(i).width();
            if (pos < (width + visualWidth + itemSpacing)) {
                pos -= width;
                return DropIndex {
                    .index = i,
                    .isOnItem = pos <= visualWidth,
                    .isBefore = pos < (visualWidth / 2)
                };
            }
            width += (visualWidth + itemSpacing);
        }
        return DropIndex { .index = m_visualItemCount - 1 };
    } else {
        int pos = point.y();
        int height = 0;
        for (int i = 0; i <= m_visualItemCount; i++) {
            int visualHeight = visualItemSize(i).height();
            if (pos < (height + visualHeight + itemSpacing)) {
                pos -= height;
                return DropIndex {
                    .index = i,
                    .isOnItem = pos <= visualHeight,
                    .isBefore = pos < (visualHeight / 2)
                };
            }
            height += (visualHeight + itemSpacing);
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

void TrayItemPositionManager::clearRegisteredSizes()
{
    // Clear visual-index mapping only. Keep surface size cache so a layout rebuild
    // can reuse the last known size immediately instead of falling back to defaults.
    if (m_registeredItemsSize.isEmpty() && m_registeredItemSurfaceIds.isEmpty()) {
        return;
    }

    m_registeredItemsSize.clear();
    m_registeredItemSurfaceIds.clear();
    notifyVisualItemSizeChanged();
}

TrayItemPositionManager::TrayItemPositionManager(QObject *parent)
    : QObject(parent)
{
    m_itemSpacing = itemSpacing;
    m_itemPadding = itemPadding;
    m_itemVisualSize = itemVisualSize;

    connect(this, &TrayItemPositionManager::visualItemCountChanged,
            this, &TrayItemPositionManager::updateVisualSize);
    connect(this, &TrayItemPositionManager::dockHeightChanged,
            this, &TrayItemPositionManager::updateVisualSize);
    connect(this, &TrayItemPositionManager::orientationChanged,
            this, &TrayItemPositionManager::updateVisualSize);
    connect(this, &TrayItemPositionManager::visualItemSizeChanged,
            this, &TrayItemPositionManager::updateVisualSize);
}

void TrayItemPositionManager::updateVisualSize()
{
    if (m_dockHeight == 0) return;
    QSize result(visualSize(m_visualItemCount - 1, false));
    qDebug() << "updateVisualSize()" << m_dockHeight << result;
    setProperty("visualSize", result);
}

void TrayItemPositionManager::ensureRegisteredItemCapacity(int count)
{
    while (m_registeredItemsSize.count() < count) {
        m_registeredItemsSize.append(itemVisualSize);
        m_registeredItemSurfaceIds.append(QString());
    }
}

void TrayItemPositionManager::notifyVisualItemSizeChanged()
{
    if (m_layoutSyncActive) {
        m_visualItemSizeChangedPending = true;
        return;
    }

    emit visualItemSizeChanged();
}

}
