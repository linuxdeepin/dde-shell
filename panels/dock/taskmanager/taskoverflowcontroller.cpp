// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "taskoverflowcontroller.h"

#include "globals.h"

#include <QTimer>
#include <QVariantMap>

#include <cmath>

namespace dock {
namespace {
constexpr qreal OverflowReleaseHysteresis = 8;
}

TaskOverflowController::TaskOverflowController(QObject *parent)
    : QObject(parent)
{
}

void TaskOverflowController::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    if (!m_enabled)
        m_latched = false;
    emit enabledChanged();
    scheduleSync();
}

void TaskOverflowController::setAvailableExtent(qreal availableExtent)
{
    if (!qFuzzyCompare(m_availableExtent + 1, availableExtent + 1)) {
        m_availableExtent = availableExtent;
        scheduleSync();
    }
}

void TaskOverflowController::setMinimumReached(bool minimumReached)
{
    if (m_minimumReached != minimumReached) {
        m_minimumReached = minimumReached;
        scheduleSync();
    }
}

void TaskOverflowController::setFallbackItemExtent(qreal fallbackItemExtent)
{
    if (!qFuzzyCompare(m_fallbackItemExtent + 1, fallbackItemExtent + 1)) {
        m_fallbackItemExtent = fallbackItemExtent;
        emit fallbackItemExtentChanged();
        scheduleSync();
    }
}

void TaskOverflowController::setSpacing(qreal spacing)
{
    if (!qFuzzyCompare(m_spacing + 1, spacing + 1)) {
        m_spacing = spacing;
        emit spacingChanged();
        scheduleSync();
    }
}

void TaskOverflowController::setVisualModel(QObject *visualModel)
{
    if (m_visualModel == visualModel)
        return;

    if (m_visualModel)
        disconnect(m_visualModel, nullptr, this, nullptr);

    m_visualModel = visualModel;
    if (m_visualModel)
        connect(m_visualModel, &QObject::destroyed, this, [this] {
            m_visualModel = nullptr;
            scheduleSync();
        });

    scheduleSync();
}

void TaskOverflowController::setDataModel(QAbstractItemModel *dataModel)
{
    if (m_dataModel == dataModel)
        return;

    disconnectDataModelSignals();
    m_dataModel = dataModel;
    connectDataModelSignals();
    scheduleSync();
}

void TaskOverflowController::scheduleSync()
{
    if (m_syncPending)
        return;

    m_syncPending = true;
    QTimer::singleShot(0, this, [this] {
        m_syncPending = false;
        sync();
    });
}

void TaskOverflowController::sync()
{
    const int totalCount = modelCount();
    const qreal fullExtent = totalCount > 0
        ? totalCount * m_fallbackItemExtent + (totalCount - 1) * m_spacing
        : 0;

    const bool overflowEnabled = syncEnabledState(totalCount, fullExtent);
    if (!overflowEnabled || totalCount <= 0 || !std::isfinite(m_availableExtent) || m_availableExtent <= 0) {
        setVisibleItemCount(totalCount);
        setPopupItems({});
        return;
    }

    qreal usedExtent = 0;
    int visibleCount = 0;
    for (int index = 0; index < totalCount; ++index) {
        const qreal itemExtent = m_fallbackItemExtent;
        const qreal leadingSpacing = visibleCount > 0 ? m_spacing : 0;
        const int remainingCount = totalCount - (visibleCount + 1);
        const qreal reservedButtonExtent = remainingCount > 0 ? m_spacing + m_fallbackItemExtent : 0;
        if (usedExtent + leadingSpacing + itemExtent + reservedButtonExtent > m_availableExtent)
            break;

        usedExtent += leadingSpacing + itemExtent;
        ++visibleCount;
    }

    if (visibleCount >= totalCount) {
        setVisibleItemCount(totalCount);
        setPopupItems({});
        return;
    }

    QVariantList overflowItems;
    overflowItems.reserve(totalCount - visibleCount);
    for (int index = visibleCount; index < totalCount; ++index) {
        const auto itemData = entryAt(index);
        if (!itemData.isEmpty())
            overflowItems.append(itemData);
    }

    setVisibleItemCount(visibleCount);
    setPopupItems(overflowItems);
}

void TaskOverflowController::onDataModelDestroyed()
{
    m_dataModel = nullptr;
    scheduleSync();
}

int TaskOverflowController::modelCount() const
{
    if (m_visualModel) {
        const auto countValue = m_visualModel->property("count");
        if (countValue.isValid())
            return countValue.toInt();
    }

    return m_dataModel ? m_dataModel->rowCount() : 0;
}

QModelIndex TaskOverflowController::modelIndexAt(int visualIndex) const
{
    if (visualIndex < 0)
        return {};

    if (m_visualModel) {
        QVariant modelIndexValue;
        const bool ok = QMetaObject::invokeMethod(m_visualModel,
                                                  "modelIndex",
                                                  Q_RETURN_ARG(QVariant, modelIndexValue),
                                                  Q_ARG(int, visualIndex));
        if (ok) {
            const auto index = modelIndexValue.value<QModelIndex>();
            if (index.isValid())
                return index;
        }
    }

    if (m_dataModel && visualIndex < m_dataModel->rowCount())
        return m_dataModel->index(visualIndex, 0);

    return {};
}

QVariant TaskOverflowController::dataForRole(const QModelIndex &index, const QByteArray &roleName) const
{
    if (!m_dataModel || !index.isValid())
        return {};

    const auto role = m_dataModel->roleNames().key(roleName, -1);
    if (role < 0)
        return {};

    return m_dataModel->data(index, role);
}

QVariantMap TaskOverflowController::entryAt(int visualIndex) const
{
    const auto index = modelIndexAt(visualIndex);
    if (!index.isValid())
        return {};

    auto iconName = dataForRole(index, MODEL_ICONNAME).toString();
    if (iconName.isEmpty())
        iconName = dataForRole(index, MODEL_WINICON).toString();
    if (iconName.isEmpty())
        iconName = DEFAULT_APP_ICONNAME;

    return {
        { QStringLiteral("active"), dataForRole(index, MODEL_ACTIVE).toBool() },
        { QStringLiteral("attention"), dataForRole(index, MODEL_ATTENTION).toBool() },
        { QStringLiteral("docked"), dataForRole(index, MODEL_DOCKED).toBool() },
        { QStringLiteral("itemId"), dataForRole(index, MODEL_ITEMID).toString() },
        { QStringLiteral("name"), dataForRole(index, MODEL_NAME).toString() },
        { QStringLiteral("title"), dataForRole(index, MODEL_TITLE).toString() },
        { QStringLiteral("iconName"), iconName },
        { QStringLiteral("menus"), dataForRole(index, MODEL_MENUS).toString() },
        { QStringLiteral("windows"), dataForRole(index, MODEL_WINDOWS).toStringList() },
        { QStringLiteral("groupItems"), dataForRole(index, MODEL_GROUPITEMS).toList() },
        { QStringLiteral("visualIndex"), visualIndex },
        { QStringLiteral("modelIndex"), QVariant::fromValue(index) },
    };
}

bool TaskOverflowController::syncEnabledState(int totalCount, qreal fullExtent)
{
    if (!m_enabled || totalCount <= 0 || !std::isfinite(m_availableExtent) || m_availableExtent <= 0) {
        m_latched = false;
        return false;
    }

    // Enter overflow as soon as the content exceeds the available boundary.
    // Keep hysteresis only when leaving overflow to avoid rapid toggling near
    // the threshold without allowing visible items to overlap first.
    const qreal startThreshold = m_availableExtent;
    const qreal stopThreshold = std::max<qreal>(0, m_availableExtent - OverflowReleaseHysteresis);
    const bool nextLatched = m_latched
        ? fullExtent > stopThreshold
        : (m_minimumReached && fullExtent > startThreshold);
    m_latched = nextLatched;
    return m_latched;
}

void TaskOverflowController::connectDataModelSignals()
{
    if (!m_dataModel)
        return;

    connect(m_dataModel, &QAbstractItemModel::dataChanged, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::rowsInserted, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::rowsMoved, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::rowsRemoved, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::modelReset, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::layoutChanged, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QObject::destroyed, this, &TaskOverflowController::onDataModelDestroyed);
}

void TaskOverflowController::disconnectDataModelSignals()
{
    if (m_dataModel)
        disconnect(m_dataModel, nullptr, this, nullptr);
}

void TaskOverflowController::setVisibleItemCount(int visibleItemCount)
{
    if (m_visibleItemCount == visibleItemCount)
        return;

    m_visibleItemCount = visibleItemCount;
    emit visibleItemCountChanged();
}

void TaskOverflowController::setPopupItems(const QVariantList &items)
{
    if (m_popupItems == items)
        return;

    m_popupItems = items;
    emit popupItemsChanged();
}

}
