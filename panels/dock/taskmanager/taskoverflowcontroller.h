// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>
#include <QPersistentModelIndex>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

namespace dock {

class TaskOverflowModel : public QAbstractListModel
{
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setItems(const QVariantList &items);

private:
    struct Entry {
        QPersistentModelIndex source;
        QVariantMap values;
    };
    QList<Entry> m_entries;
};

class TaskOverflowController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal availableExtent READ availableExtent WRITE setAvailableExtent)
    Q_PROPERTY(bool minimumReached READ minimumReached WRITE setMinimumReached)
    Q_PROPERTY(qreal fallbackItemExtent READ fallbackItemExtent WRITE setFallbackItemExtent NOTIFY fallbackItemExtentChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(qreal overflowSidePadding READ overflowSidePadding WRITE setOverflowSidePadding NOTIFY overflowSidePaddingChanged)
    Q_PROPERTY(QObject *visualModel READ visualModel WRITE setVisualModel)
    Q_PROPERTY(QAbstractItemModel *dataModel READ dataModel WRITE setDataModel)
    Q_PROPERTY(int visibleItemCount READ visibleItemCount NOTIFY visibleItemCountChanged)
    Q_PROPERTY(QAbstractItemModel *popupModel READ popupModel CONSTANT)
    Q_PROPERTY(int popupCount READ popupCount NOTIFY popupItemsChanged)
    Q_PROPERTY(QVariantMap primaryItem READ primaryItem NOTIFY popupItemsChanged)
    Q_PROPERTY(QVariantMap recentItem READ recentItem NOTIFY recentItemChanged)
    Q_PROPERTY(bool showRecent READ showRecent NOTIFY layoutChanged)
    Q_PROPERTY(qreal trailingExtent READ trailingExtent NOTIFY layoutChanged)
    Q_PROPERTY(qreal buttonOffset READ buttonOffset NOTIFY layoutChanged)
    Q_PROPERTY(qreal buttonExtent READ buttonExtent NOTIFY layoutChanged)
    Q_PROPERTY(qreal separatorPadding READ separatorPadding NOTIFY layoutChanged)
    Q_PROPERTY(bool dragInProgress READ dragInProgress NOTIFY dragInProgressChanged)

public:
    explicit TaskOverflowController(QObject *parent = nullptr);

    bool isEnabled() const
    {
        return m_enabled;
    }
    void setEnabled(bool enabled);

    qreal availableExtent() const
    {
        return m_availableExtent;
    }
    void setAvailableExtent(qreal availableExtent);

    bool minimumReached() const
    {
        return m_minimumReached;
    }
    void setMinimumReached(bool minimumReached);

    qreal fallbackItemExtent() const
    {
        return m_fallbackItemExtent;
    }
    void setFallbackItemExtent(qreal fallbackItemExtent);

    qreal spacing() const
    {
        return m_spacing;
    }
    void setSpacing(qreal spacing);

    qreal overflowSidePadding() const { return m_overflowSidePadding; }
    void setOverflowSidePadding(qreal padding);

    QObject *visualModel() const
    {
        return m_visualModel;
    }
    void setVisualModel(QObject *visualModel);

    QAbstractItemModel *dataModel() const
    {
        return m_dataModel;
    }
    void setDataModel(QAbstractItemModel *dataModel);

    int visibleItemCount() const
    {
        return m_visibleItemCount;
    }
    QVariantList popupItems() const
    {
        return m_popupItems;
    }
    QVariantMap recentItem() const { return m_recentItem; }
    QAbstractItemModel *popupModel() { return &m_popupModel; }
    int popupCount() const { return m_popupItems.size(); }
    QVariantMap primaryItem() const { return m_popupItems.value(0).toMap(); }
    bool showRecent() const { return m_showRecent; }
    qreal trailingExtent() const { return m_trailingExtent; }
    qreal buttonOffset() const { return m_buttonOffset; }
    qreal buttonExtent() const { return m_buttonExtent; }
    qreal separatorPadding() const { return m_separatorPadding; }
    bool dragInProgress() const { return m_dragInProgress; }

    Q_INVOKABLE void scheduleSync();
    Q_INVOKABLE void beginDrag();
    Q_INVOKABLE bool finishDrag(int dropAction);

Q_SIGNALS:
    void enabledChanged();
    void fallbackItemExtentChanged();
    void spacingChanged();
    void overflowSidePaddingChanged();
    void visibleItemCountChanged();
    void popupItemsChanged();
    void recentItemChanged();
    void layoutChanged();
    void dragInProgressChanged();

private Q_SLOTS:
    void onDataModelDestroyed();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void sync();
    int modelCount() const;
    QModelIndex modelIndexAt(int visualIndex) const;
    QVariant dataForRole(const QModelIndex &index, const QByteArray &roleName) const;
    QVariantMap entryAt(int visualIndex);
    bool syncEnabledState(int totalCount, qreal fullExtent);

    void connectDataModelSignals();
    void disconnectDataModelSignals();

    void setVisibleItemCount(int visibleItemCount);
    void setPopupItems(const QVariantList &items);
    void cacheRoles();
    void captureActivity(bool baseline = false, int first = 0, int last = -1);
    void syncRecentItem(const QVariantList &items);
    void setLayout(bool showRecent, qreal trailingExtent, qreal buttonOffset,
                   qreal buttonExtent, qreal separatorPadding);

    bool m_enabled = true;
    qreal m_availableExtent = 0;
    bool m_minimumReached = false;
    qreal m_fallbackItemExtent = 1;
    qreal m_spacing = 0;
    qreal m_overflowSidePadding = 0;
    QPointer<QObject> m_visualModel;
    QPointer<QAbstractItemModel> m_dataModel;
    bool m_latched = false;
    int m_visibleItemCount = 0;
    QVariantList m_popupItems;
    TaskOverflowModel m_popupModel;
    QHash<QByteArray, int> m_roles;
    QSet<int> m_entryRoles;
    QHash<QPersistentModelIndex, QVariantMap> m_entryCache;
    struct Activity {
        QString id;
        bool active;
        bool running;
    };
    QHash<QPersistentModelIndex, Activity> m_activity;
    QVariantMap m_recentItem;
    QStringList m_recentHistory; // Oldest first, one record per application.
    QStringList m_pendingActivity;
    QSet<QString> m_activeItems;
    QSet<QString> m_runningItems;
    bool m_syncPending = false;
    bool m_dragInProgress = false;
    bool m_dragCancelled = false;
    bool m_syncDeferred = false;
    bool m_showRecent = false;
    qreal m_trailingExtent = 0;
    qreal m_buttonOffset = 0;
    qreal m_buttonExtent = 0;
    qreal m_separatorPadding = 0;
};

}
