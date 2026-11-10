// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "taskoverflowcontroller.h"

#include "globals.h"

#include <QTimer>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <utility>

namespace dock {
namespace {
constexpr qreal OverflowReleaseHysteresis = 8;
}

int TaskOverflowModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant TaskOverflowModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size() || role != Qt::UserRole)
        return {};
    return m_entries.at(index.row()).values;
}

QHash<int, QByteArray> TaskOverflowModel::roleNames() const
{
    return {{Qt::UserRole, "entry"}};
}

void TaskOverflowModel::setItems(const QVariantList &items)
{
    for (int row = 0; row < items.size(); ++row) {
        const auto values = items.at(row).toMap();
        const QPersistentModelIndex source = values.value(QStringLiteral("modelIndex")).value<QModelIndex>();
        int existing = row;
        while (existing < m_entries.size() && m_entries.at(existing).source != source)
            ++existing;
        if (existing == m_entries.size()) {
            beginInsertRows({}, row, row);
            m_entries.insert(row, {source, values});
            endInsertRows();
        } else {
            if (existing != row) {
                beginMoveRows({}, existing, existing, {}, row);
                m_entries.move(existing, row);
                endMoveRows();
            }
            if (m_entries.at(row).values != values) {
                m_entries[row].values = values;
                emit dataChanged(index(row), index(row), {Qt::UserRole});
            }
        }
    }
    if (m_entries.size() > items.size()) {
        beginRemoveRows({}, items.size(), m_entries.size() - 1);
        m_entries.erase(m_entries.begin() + items.size(), m_entries.end());
        endRemoveRows();
    }
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
    availableExtent = std::isfinite(availableExtent) ? std::max<qreal>(0, availableExtent) : 0;
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
    fallbackItemExtent = std::isfinite(fallbackItemExtent) ? std::max<qreal>(1, fallbackItemExtent) : 1;
    if (!qFuzzyCompare(m_fallbackItemExtent + 1, fallbackItemExtent + 1)) {
        m_fallbackItemExtent = fallbackItemExtent;
        emit fallbackItemExtentChanged();
        scheduleSync();
    }
}

void TaskOverflowController::setSpacing(qreal spacing)
{
    spacing = std::isfinite(spacing) ? std::max<qreal>(0, spacing) : 0;
    if (!qFuzzyCompare(m_spacing + 1, spacing + 1)) {
        m_spacing = spacing;
        emit spacingChanged();
        scheduleSync();
    }
}

void TaskOverflowController::setOverflowSidePadding(qreal padding)
{
    padding = std::isfinite(padding) ? std::max<qreal>(0, padding) : 0;
    if (qFuzzyCompare(m_overflowSidePadding + 1, padding + 1))
        return;

    m_overflowSidePadding = padding;
    emit overflowSidePaddingChanged();
    scheduleSync();
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
    cacheRoles();
    m_entryCache.clear();
    m_recentHistory.clear();
    m_pendingActivity.clear();
    captureActivity(true);
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

void TaskOverflowController::beginDrag()
{
    if (m_dragInProgress)
        return;
    m_dragInProgress = true;
    emit dragInProgressChanged();
    m_dragCancelled = false;
    // Install after QDrag starts its event loop so that Escape is observed
    // before the native drag filter consumes it and clears the mouse state.
    QTimer::singleShot(0, this, [this] {
        if (m_dragInProgress)
            QCoreApplication::instance()->installEventFilter(this);
    });
}

bool TaskOverflowController::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
        m_dragCancelled = true;
    return QObject::eventFilter(watched, event);
}

bool TaskOverflowController::finishDrag(int dropAction)
{
    if (!m_dragInProgress)
        return false;
    m_dragInProgress = false;
    QCoreApplication::instance()->removeEventFilter(this);
    emit dragInProgressChanged();
    if (m_syncDeferred) {
        m_syncDeferred = false;
        scheduleSync();
    }
    // IgnoreAction represents both dropping outside and cancelling with Escape.
    return dropAction == Qt::IgnoreAction && !m_dragCancelled
        && !(QGuiApplication::mouseButtons() & Qt::LeftButton);
}

void TaskOverflowController::setLayout(bool showRecent, qreal trailingExtent, qreal buttonOffset,
                                       qreal buttonExtent, qreal separatorPadding)
{
    if (m_showRecent == showRecent && m_trailingExtent == trailingExtent
        && m_buttonOffset == buttonOffset && m_buttonExtent == buttonExtent
        && m_separatorPadding == separatorPadding)
        return;
    m_showRecent = showRecent;
    m_trailingExtent = trailingExtent;
    m_buttonOffset = buttonOffset;
    m_buttonExtent = buttonExtent;
    m_separatorPadding = separatorPadding;
    emit layoutChanged();
}

void TaskOverflowController::sync()
{
    // A native drag runs a nested event loop. Keep its source delegate alive
    // until it returns, even if a window opens/closes or the dock is resized.
    if (m_dragInProgress) {
        m_syncDeferred = true;
        return;
    }
    const int totalCount = modelCount();
    const qreal fullExtent = totalCount > 0
        ? totalCount * m_fallbackItemExtent + (totalCount - 1) * m_spacing : 0;
    if (!syncEnabledState(totalCount, fullExtent)) {
        setLayout(false, 0, 0, 0, 0);
        setVisibleItemCount(totalCount);
        setPopupItems({});
        return;
    }

    const qreal available = std::max<qreal>(0, m_availableExtent);
    const qreal pairExtent = 2 * (m_fallbackItemExtent + m_overflowSidePadding) + m_spacing;
    const bool showRecent = pairExtent <= available;
    const qreal buttonExtent = std::min(m_fallbackItemExtent, available);
    const qreal padding = std::min(m_overflowSidePadding, (available - buttonExtent) / 2);
    const qreal groupExtent = showRecent ? pairExtent : buttonExtent + 2 * padding;
    const qreal stride = m_fallbackItemExtent + m_spacing;
    const int visibleCount = std::clamp(int(std::floor((available - groupExtent) / stride)), 0, totalCount - 1);
    const qreal buttonOffset = visibleCount * stride + padding + (showRecent ? stride : 0);

    QVariantList overflowItems;
    overflowItems.reserve(totalCount - visibleCount);
    for (int index = visibleCount; index < totalCount; ++index) {
        const auto itemData = entryAt(index);
        if (!itemData.isEmpty())
            overflowItems.append(itemData);
    }
    setLayout(showRecent, groupExtent, buttonOffset, buttonExtent, padding);
    setVisibleItemCount(visibleCount);
    setPopupItems(overflowItems);
}

void TaskOverflowController::onDataModelDestroyed()
{
    m_dataModel = nullptr;
    m_entryCache.clear();
    m_activity.clear();
    m_activeItems.clear();
    m_runningItems.clear();
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
                                                  "sourceIndexAt",
                                                  Q_RETURN_ARG(QVariant, modelIndexValue),
                                                  Q_ARG(QVariant, visualIndex));
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

    const auto role = m_roles.value(roleName, -1);
    if (role < 0)
        return {};

    return m_dataModel->data(index, role);
}

QVariantMap TaskOverflowController::entryAt(int visualIndex)
{
    const auto index = modelIndexAt(visualIndex);
    if (!index.isValid())
        return {};

    auto cached = m_entryCache.constFind(index);
    if (cached == m_entryCache.cend()) {
        auto iconName = dataForRole(index, MODEL_ICONNAME).toString();
        if (iconName.isEmpty())
            iconName = dataForRole(index, MODEL_WINICON).toString();
        if (iconName.isEmpty())
            iconName = DEFAULT_APP_ICONNAME;

        QVariantMap values {
            { QStringLiteral("active"), dataForRole(index, MODEL_ACTIVE).toBool() },
            { QStringLiteral("attention"), dataForRole(index, MODEL_ATTENTION).toBool() },
            { QStringLiteral("docked"), dataForRole(index, MODEL_DOCKED).toBool() },
            { QStringLiteral("itemId"), dataForRole(index, MODEL_ITEMID).toString() },
            { QStringLiteral("name"), dataForRole(index, MODEL_NAME).toString() },
            { QStringLiteral("title"), dataForRole(index, MODEL_TITLE).toString() },
            { QStringLiteral("iconName"), iconName },
            { QStringLiteral("menus"), dataForRole(index, MODEL_MENUS).toString() },
            { QStringLiteral("windows"), dataForRole(index, MODEL_WINDOWS).toStringList() },
            { QStringLiteral("folderType"), dataForRole(index, MODEL_FOLDERTYPE).toString() },
            { QStringLiteral("applicationFolderContents"), dataForRole(index, MODEL_APPLICATIONFOLDERCONTENTS).toList() },
            { QStringLiteral("folderPresentation"), dataForRole(index, MODEL_FOLDERPRESENTATION) },
        };
        cached = m_entryCache.insert(index, values);
    }
    auto result = *cached;
    result.insert(QStringLiteral("visualIndex"), visualIndex);
    result.insert(QStringLiteral("modelIndex"), QVariant::fromValue(index));
    return result;
}

bool TaskOverflowController::syncEnabledState(int totalCount, qreal fullExtent)
{
    if (!m_enabled || totalCount <= 0 || !std::isfinite(m_availableExtent)) {
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

    const auto structureChanged = [this] {
        m_entryCache.clear();
        captureActivity();
        scheduleSync();
    };
    connect(m_dataModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &first, const QModelIndex &last, const QList<int> &roles) {
        const auto changed = [this, &roles](const QByteArray &name) {
            return roles.isEmpty() || roles.contains(m_roles.value(name, -1));
        };
        if (!roles.isEmpty() && std::none_of(roles.cbegin(), roles.cend(), [this](int role) {
                return m_entryRoles.contains(role);
            }))
            return;
        for (int row = first.row(); row <= last.row(); ++row)
            m_entryCache.remove(m_dataModel->index(row, 0));
        if (changed(MODEL_ACTIVE) || changed(MODEL_WINDOWS) || changed(MODEL_ITEMID))
            captureActivity(false, first.row(), last.row());
        scheduleSync();
    });
    connect(m_dataModel, &QAbstractItemModel::rowsInserted, this, structureChanged);
    connect(m_dataModel, &QAbstractItemModel::rowsMoved, this, &TaskOverflowController::scheduleSync);
    connect(m_dataModel, &QAbstractItemModel::rowsRemoved, this, structureChanged);
    connect(m_dataModel, &QAbstractItemModel::modelReset, this, [this, structureChanged] {
        cacheRoles();
        structureChanged();
    });
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
    syncRecentItem(items);
    if (m_popupItems == items)
        return;

    m_popupItems = items;
    m_popupModel.setItems(items);
    emit popupItemsChanged();
}

void TaskOverflowController::cacheRoles()
{
    m_roles.clear();
    m_entryRoles.clear();
    if (!m_dataModel)
        return;
    const auto roles = m_dataModel->roleNames();
    for (const QByteArray name : {MODEL_ICONNAME, MODEL_WINICON, MODEL_ACTIVE, MODEL_ATTENTION,
                                  MODEL_DOCKED, MODEL_ITEMID, MODEL_NAME, MODEL_TITLE, MODEL_MENUS, MODEL_WINDOWS,
                                  MODEL_FOLDERTYPE, MODEL_APPLICATIONFOLDERCONTENTS, MODEL_FOLDERPRESENTATION}) {
        const int role = roles.key(name, -1);
        if (role >= 0) {
            m_roles.insert(name, role);
            m_entryRoles.insert(role);
        }
    }
}

void TaskOverflowController::captureActivity(bool baseline, int first, int last)
{
    if (last < 0) {
        m_activity.clear();
        last = m_dataModel ? m_dataModel->rowCount() - 1 : -1;
    }
    QStringList changedIds;
    for (int row = first; m_dataModel && row <= last; ++row) {
        const auto index = m_dataModel->index(row, 0);
        Activity state {dataForRole(index, MODEL_ITEMID).toString(),
                        dataForRole(index, MODEL_ACTIVE).toBool(),
                        !dataForRole(index, MODEL_WINDOWS).toStringList().isEmpty()};
        m_activity.insert(index, state);
        if (!state.id.isEmpty() && !changedIds.contains(state.id))
            changedIds.append(state.id);
    }
    // Aggregate cached rows because split windows can share an application ID.
    QSet<QString> activeItems;
    QSet<QString> runningItems;
    for (const auto &state : std::as_const(m_activity)) {
        if (state.active)
            activeItems.insert(state.id);
        if (state.running)
            runningItems.insert(state.id);
    }
    if (!baseline) {
        for (const auto &id : changedIds) {
            if ((activeItems.contains(id) && !m_activeItems.contains(id))
                || (runningItems.contains(id) && !m_runningItems.contains(id))) {
                m_pendingActivity.removeAll(id);
                m_pendingActivity.append(id);
            }
        }
    }
    m_activeItems = activeItems;
    m_runningItems = runningItems;
}

void TaskOverflowController::syncRecentItem(const QVariantList &items)
{
    const bool recentClosed = !m_recentItem.value(QStringLiteral("windows")).toStringList().isEmpty()
        && !m_runningItems.contains(m_recentItem.value(QStringLiteral("itemId")).toString());
    QHash<QString, QVariantMap> applications;
    for (const auto &item : items) {
        const auto entry = item.toMap();
        const auto id = entry.value(QStringLiteral("itemId")).toString();
        if (id.isEmpty())
            continue;
        // In window-split mode prefer the active, then a running window of an app.
        const auto previous = applications.value(id);
        if (previous.isEmpty() || entry.value(QStringLiteral("active")).toBool()
            || (!previous.value(QStringLiteral("active")).toBool()
                && previous.value(QStringLiteral("windows")).toStringList().isEmpty()))
            applications.insert(id, entry);
    }
    const auto eligible = [&applications](const QString &id) {
        const auto entry = applications.value(id);
        return !entry.isEmpty() && (entry.value(QStringLiteral("docked")).toBool()
                                   || !entry.value(QStringLiteral("windows")).toStringList().isEmpty());
    };
    for (auto it = m_recentHistory.begin(); it != m_recentHistory.end();) {
        if (!eligible(*it))
            it = m_recentHistory.erase(it);
        else
            ++it;
    }
    // A background app can close while another overflow app remains active,
    // without producing a new active-role transition for that fallback app.
    if (recentClosed) {
        for (const auto &item : items) {
            const auto entry = item.toMap();
            if (entry.value(QStringLiteral("active")).toBool())
                m_pendingActivity.append(entry.value(QStringLiteral("itemId")).toString());
        }
    }
    for (const auto &id : std::as_const(m_pendingActivity)) {
        if (!eligible(id))
            continue;
        m_recentHistory.removeAll(id);
        m_recentHistory.append(id);
        if (m_recentHistory.size() > 30)
            m_recentHistory.erase(m_recentHistory.begin(), m_recentHistory.begin() + 10);
    }
    m_pendingActivity.clear();

    QVariantMap recent;
    // Closing the current app falls back to the next running record, even if
    // the closed app is pinned and therefore still present in the history.
    for (auto it = m_recentHistory.crbegin(); it != m_recentHistory.crend(); ++it) {
        const auto entry = applications.value(*it);
        if (!entry.value(QStringLiteral("windows")).toStringList().isEmpty()) {
            recent = entry;
            break;
        }
    }
    if (recent.isEmpty() && !m_recentHistory.isEmpty()) {
        const auto previousId = m_recentItem.value(QStringLiteral("itemId")).toString();
        if (!recentClosed && m_recentHistory.contains(previousId)) {
            recent = applications.value(previousId);
        } else {
            for (auto it = m_recentHistory.crbegin(); it != m_recentHistory.crend(); ++it) {
                if (*it != previousId) {
                    recent = applications.value(*it);
                    break;
                }
            }
        }
        if (recent.isEmpty())
            recent = applications.value(m_recentHistory.last());
    }
    if (recent.isEmpty() && !items.isEmpty())
        recent = items.last().toMap();
    if (m_recentItem != recent) {
        m_recentItem = recent;
        emit recentItemChanged();
    }
}

}
